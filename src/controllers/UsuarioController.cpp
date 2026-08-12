#include "controllers/UsuarioController.h"
#include "middleware/Middleware.h"
#include "services/UsuarioService.h"
#include "session/SessionManager.h"
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// POST /api/usuarios
// ---------------------------------------------------------------------------
static void handleCrear(AsyncWebServerRequest *request, uint8_t *data,
                        size_t len, size_t, size_t)
{
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, data, len))
    {
        enviarError(request, 400, "JSON inválido.");
        return;
    }

    Usuario u;
    u.usuario = doc["usuario"] | "";
    u.nombre = doc["nombre"] | "";
    u.apellido = doc["apellido"] | "";
    u.rol = doc["rol"] | "";
    u.referencia = doc["referencia"] | "";
    u.contacto = doc["contacto"] | "";

    String password = doc["password"] | "";
    String confirmar = doc["confirmar"] | "";
    String claveMaestra = doc["claveMaestra"] | "";

    UsuarioResult result = UsuarioService::getInstance().crear(u, password,
                                                               confirmar,
                                                               claveMaestra);
    if (!result.ok)
    {
        enviarError(request, 400, result.mensaje);
        return;
    }

    StaticJsonDocument<128> resp;
    resp["ok"] = true;
    resp["idUsuario"] = result.id;
    resp["mensaje"] = "Usuario creado correctamente.";

    String json;
    serializeJson(resp, json);
    enviarJSON(request, 201, json);
}

// ---------------------------------------------------------------------------
// DELETE /api/usuarios/:usuario
// ---------------------------------------------------------------------------
static void handleEliminar(AsyncWebServerRequest *request)
{
    if (!verificarSesionPanel(request)) return;
    
    if (!request->hasParam("id")) {
        enviarError(request, 400, "Falta el ID del usuario.");
        return;
    }
    
    int idUsuario = request->getParam("id")->value().toInt();

    // Llama al único servicio
    UsuarioResult result = UsuarioService::getInstance().eliminar(idUsuario);
    
    if (!result.ok) {
        enviarError(request, 400, result.mensaje);
        return;
    }

    enviarOk(request, "Usuario eliminado exitosamente.");
}

// ---------------------------------------------------------------------------
// GET /api/usuarios/perfil
// ---------------------------------------------------------------------------
static void handleObtenerPerfil(AsyncWebServerRequest *request) {
    if (!verificarSesionPanel(request)) return;

    const SesionPanel &sesion = SessionManager::getInstance().getSesionPanel();
    if (sesion.rol == "invitado") {
        enviarError(request, 403, "El invitado no tiene perfil.");
        return;
    }

    Usuario u = UsuarioService::getInstance().obtenerPerfil(sesion.idUsuario);
    if (u.idUsuario == 0) {
        enviarError(request, 404, "Usuario no encontrado.");
        return;
    }

    StaticJsonDocument<256> resp;
    resp["idUsuario"] = u.idUsuario;
    resp["usuario"] = u.usuario;
    resp["nombre"] = u.nombre;
    resp["apellido"] = u.apellido;
    resp["referencia"] = u.referencia;
    resp["contacto"] = u.contacto;

    String json;
    serializeJson(resp, json);
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// PUT /api/usuarios/perfil
// ---------------------------------------------------------------------------
static void handleEditarPerfil(AsyncWebServerRequest *request, uint8_t *data,
                               size_t len, size_t, size_t)
{
    if (!verificarSesionPanel(request))
        return;

    const SesionPanel &sesion = SessionManager::getInstance().getSesionPanel();
    if (sesion.rol == "invitado")
    {
        enviarError(request, 403, "El invitado no tiene perfil para editar.");
        return;
    }

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, data, len))
    {
        enviarError(request, 400, "JSON inválido.");
        return;
    }

    Usuario u = UsuarioService::getInstance().obtenerPerfil(sesion.idUsuario);
    if (u.idUsuario == 0)
    {
        enviarError(request, 404, "Usuario no encontrado.");
        return;
    }

    u.usuario = doc["usuario"] | u.usuario;
    u.nombre = doc["nombre"] | u.nombre;
    u.apellido = doc["apellido"] | u.apellido;
    u.referencia = doc["referencia"] | u.referencia;
    u.contacto = doc["contacto"] | u.contacto;

    UsuarioResult result = UsuarioService::getInstance().editarPerfil(u);
    if (!result.ok)
    {
        enviarError(request, 400, result.mensaje);
        return;
    }

    String nombreCompleto = u.nombre + " " + u.apellido;
    SessionManager::getInstance().actualizarNombrePanel(nombreCompleto);

    enviarOk(request, "Perfil actualizado correctamente.");
}

// ---------------------------------------------------------------------------
// PUT /api/usuarios/password
// ---------------------------------------------------------------------------
static void handleRecuperarPassword(AsyncWebServerRequest *request, uint8_t *data,
                                  size_t len, size_t, size_t)
{
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, data, len))
    {
        enviarError(request, 400, "JSON inválido.");
        return;
    }

    String usuario = doc["usuario"] | "";
    String claveMaestra = doc["claveMaestra"] | "";
    String nuevaPassword = doc["passwordNueva"] | "";
    String confirmar = doc["confirmar"] | "";

    UsuarioResult result = UsuarioService::getInstance().recuperarPassword(
        usuario, claveMaestra, nuevaPassword, confirmar);

    if (!result.ok)
    {
        enviarError(request, 400, result.mensaje);
        return;
    }

    enviarOk(request, "Contraseña recuperada correctamente.");
}

// ---------------------------------------------------------------------------
// PUT /api/usuarios/perfil/password
// ---------------------------------------------------------------------------
static void handleCambiarPassword(AsyncWebServerRequest *request, uint8_t *data,
                                        size_t len, size_t, size_t) {
    if (!verificarSesionPanel(request)) return;

    const SesionPanel &sesion = SessionManager::getInstance().getSesionPanel();

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, data, len)) {
        enviarError(request, 400, "JSON inválido.");
        return;
    }

    String actual = doc["passwordActual"] | "";
    String nueva = doc["passwordNueva"] | "";
    String confirmar = doc["confirmar"] | "";

    UsuarioResult result = UsuarioService::getInstance().cambiarPassword(
        sesion.idUsuario, actual, nueva, confirmar);

    if (!result.ok) {
        enviarError(request, 400, result.mensaje);
        return;
    }

    enviarOk(request, "Contraseña actualizada correctamente.");
}

// ---------------------------------------------------------------------------
// GET /api/usuarios/profesores
// ---------------------------------------------------------------------------
static void handleListarProfesores(AsyncWebServerRequest *request)
{
    if (!verificarSesionPanel(request))
        return;

    const SesionPanel &sesion = SessionManager::getInstance().getSesionPanel();
    if (sesion.rol == "invitado")
    {
        enviarError(request, 403, "No tenés permisos para esta acción.");
        return;
    }

    UsuarioResumen buffer[20];
    int cant = UsuarioService::getInstance().listarProfesores(buffer, 20);

    StaticJsonDocument<2048> resp;
    resp["ok"] = true;
    JsonArray arr = resp.createNestedArray("profesores");

    for (int i = 0; i < cant; i++)
    {
        JsonObject obj = arr.createNestedObject();
        obj["idUsuario"] = buffer[i].idUsuario;
        obj["nombre"] = buffer[i].nombre;
        obj["apellido"] = buffer[i].apellido;
        obj["materia"] = buffer[i].referencia;
        obj["contacto"] = buffer[i].contacto;
    }

    String json;
    serializeJson(resp, json);
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// GET /api/usuarios/tutores
// ---------------------------------------------------------------------------
static void handleListarTutores(AsyncWebServerRequest *request)
{
    if (!verificarSesionPanel(request))
        return;

    const SesionPanel &sesion = SessionManager::getInstance().getSesionPanel();
    if (sesion.rol == "invitado")
    {
        enviarError(request, 403, "No tenés permisos para esta acción.");
        return;
    }

    UsuarioResumen buffer[20];
    int cant = UsuarioService::getInstance().listarTutores(buffer, 20);

    StaticJsonDocument<2048> resp;
    resp["ok"] = true;
    JsonArray arr = resp.createNestedArray("tutores");

    for (int i = 0; i < cant; i++)
    {
        JsonObject obj = arr.createNestedObject();
        obj["idUsuario"] = buffer[i].idUsuario;
        obj["nombre"] = buffer[i].nombre;
        obj["apellido"] = buffer[i].apellido;
        obj["vinculo"] = buffer[i].referencia;
        obj["contacto"] = buffer[i].contacto;
    }

    String json;
    serializeJson(resp, json);
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// Registro de rutas
// ---------------------------------------------------------------------------
void registrarUsuarioController(AsyncWebServer &server)
{
    server.on("/api/usuarios/perfil/password", HTTP_PUT, [](AsyncWebServerRequest *r) {}, nullptr, handleCambiarPassword);
    server.on("/api/usuarios/perfil", HTTP_PUT, [](AsyncWebServerRequest *r) {}, nullptr, handleEditarPerfil);
    server.on("/api/usuarios/perfil", HTTP_GET, handleObtenerPerfil);

    server.on("/api/usuarios/password", HTTP_PUT, [](AsyncWebServerRequest *r) {}, nullptr, handleRecuperarPassword);
    server.on("/api/usuarios", HTTP_POST, [](AsyncWebServerRequest *r) {}, nullptr, handleCrear);
    server.on("/api/usuario", HTTP_DELETE, handleEliminar);

    server.on("/api/usuarios/profesores", HTTP_GET, handleListarProfesores);
    server.on("/api/usuarios/tutores", HTTP_GET, handleListarTutores);
}