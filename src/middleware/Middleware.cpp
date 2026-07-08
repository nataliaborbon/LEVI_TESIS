#include "Middleware.h"

String extraerToken(AsyncWebServerRequest* request) {
    if (!request->hasHeader("Authorization")) return "";

    String auth = request->getHeader("Authorization")->value();

    // Formato esperado: "Bearer <token>"
    if (!auth.startsWith("Bearer ")) return "";

    return auth.substring(7); // salteamos "Bearer "
}

bool verificarSesionPanel(AsyncWebServerRequest* request, const String& rolRequerido) {
    String token = extraerToken(request);

    if (!SessionManager::getInstance().verificarTokenPanel(token)) {
        enviarError(request, 401, "Sesión inválida o expirada.");
        return false;
    }

    if (rolRequerido.length() > 0) {
        const SesionPanel& sesion = SessionManager::getInstance().getSesionPanel();
        if (sesion.rol != rolRequerido) {
            enviarError(request, 403, "No tenés permisos para esta acción.");
            return false;
        }
    }

    // Actualizar actividad de la sesión en cada request válido
    SessionManager::getInstance().heartbeatPanel(token);
    return true;
}

bool verificarSesionAlumno(AsyncWebServerRequest* request) {
    String token = extraerToken(request);

    if (!SessionManager::getInstance().verificarTokenAlumno(token)) {
        enviarError(request, 401, "Sesión de alumno inválida o expirada.");
        return false;
    }

    SessionManager::getInstance().heartbeatAlumno(token);
    return true;
}

void enviarJSON(AsyncWebServerRequest* request, int codigo, const String& json) {
    AsyncWebServerResponse* response =
        request->beginResponse(codigo, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void enviarError(AsyncWebServerRequest* request, int codigo, const String& mensaje) {
    String json = "{\"ok\":false,\"mensaje\":\"" + mensaje + "\"}";
    enviarJSON(request, codigo, json);
}

void enviarOk(AsyncWebServerRequest* request, const String& mensaje) {
    String json = "{\"ok\":true,\"mensaje\":\"" + mensaje + "\"}";
    enviarJSON(request, 200, json);
}
