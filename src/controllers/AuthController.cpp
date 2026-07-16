#include "controllers/AuthController.h"
#include "middleware/Middleware.h"
#include "services/AuthService.h"
#include "services/UsuarioService.h"
#include "session/SessionManager.h"
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// POST /api/auth/login
// Body: { "usuario": "...", "password": "..." }
// ---------------------------------------------------------------------------
static void handleLogin(AsyncWebServerRequest* request, uint8_t* data,
                         size_t len, size_t, size_t) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, data, len)) {
        enviarError(request, 400, "JSON inválido.");
        return;
    }

    String usuario  = doc["usuario"]  | "";
    String password = doc["password"] | "";

    AuthResult result = AuthService::getInstance().login(usuario, password);

    if (!result.ok) {
        enviarError(request, 401, result.mensaje);
        return;
    }

    StaticJsonDocument<256> resp;
    resp["ok"]        = true;
    resp["token"]     = result.token;
    resp["rol"]       = result.rol;
    resp["idUsuario"] = result.idUsuario;
    resp["nombre"]    = result.nombre;

    String json;
    serializeJson(resp, json);
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// POST /api/auth/invitado
// ---------------------------------------------------------------------------
static void handleLoginInvitado(AsyncWebServerRequest* request, uint8_t* data,
                                  size_t len, size_t, size_t) {
    AuthResult result = AuthService::getInstance().loginInvitado();

    if (!result.ok) {
        enviarError(request, 409, result.mensaje);
        return;
    }

    StaticJsonDocument<128> resp;
    resp["ok"]    = true;
    resp["token"] = result.token;
    resp["rol"]   = result.rol;

    String json;
    serializeJson(resp, json);
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// GET /api/auth/perfil
// Header: Authorization: Bearer <token>
// ---------------------------------------------------------------------------
static void handlePerfil(AsyncWebServerRequest* request) {
    if (!verificarSesionPanel(request)) return;

    const SesionPanel& sesion = SessionManager::getInstance().getSesionPanel();

    // Invitado no tiene perfil en BD
    if (sesion.rol == "invitado") {
        StaticJsonDocument<128> resp;
        resp["ok"]  = true;
        resp["rol"] = "invitado";
        String json;
        serializeJson(resp, json);
        enviarJSON(request, 200, json);
        return;
    }

    Usuario u = UsuarioService::getInstance().obtenerPerfil(sesion.idUsuario);
    if (u.idUsuario == 0) {
        enviarError(request, 404, "Usuario no encontrado.");
        return;
    }

    StaticJsonDocument<512> resp;
    resp["ok"]        = true;
    resp["idUsuario"] = u.idUsuario;
    resp["usuario"]   = u.usuario;
    resp["nombre"]    = u.nombre;
    resp["apellido"]  = u.apellido;
    resp["rol"]       = u.rol;
    resp["materia"]   = u.materia;
    resp["contacto"]  = u.contacto;

    String json;
    serializeJson(resp, json);
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// Registro de rutas
// ---------------------------------------------------------------------------
void registrarAuthController(AsyncWebServer& server) {
    server.on("/api/auth/login",     HTTP_POST, [](AsyncWebServerRequest* r){},
              nullptr, handleLogin);

    server.on("/api/auth/invitado",  HTTP_POST, [](AsyncWebServerRequest* r){},
              nullptr, handleLoginInvitado);

    server.on("/api/auth/logout", HTTP_POST, [](AsyncWebServerRequest* r) {
        if (!verificarSesionPanel(r)) return;
        AuthService::getInstance().logout();
        enviarOk(r, "Sesión cerrada.");
    });

    server.on("/api/auth/heartbeat", HTTP_POST, [](AsyncWebServerRequest* r) {
        if (!verificarSesionPanel(r)) return;
        enviarOk(r, "ok");
    });

    server.on("/api/auth/perfil",    HTTP_GET, handlePerfil);
}
