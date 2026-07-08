#include "RespuestaController.h"
#include "../middleware/Middleware.h"
#include "../services/RespuestaService.h"
#include "../session/SessionManager.h"
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// POST /api/alumno/iniciar
// Público: cualquier dispositivo puede intentar entrar como alumno.
// ---------------------------------------------------------------------------
static void handleAlumnoIniciar(AsyncWebServerRequest* request, uint8_t* data,
                                  size_t len, size_t, size_t) {
    SesionResult result = RespuestaService::getInstance().iniciarSesion();

    if (!result.ok) {
        enviarError(request, 409, result.mensaje);
        return;
    }

    String json = "{\"ok\":true,\"token\":\"" + result.token + "\"}";
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// GET /api/alumno/estado
// Header: Authorization: Bearer <token>
// ---------------------------------------------------------------------------
static void handleAlumnoEstado(AsyncWebServerRequest* request) {
    if (!verificarSesionAlumno(request)) return;

    EstadoAlumno estado = RespuestaService::getInstance().obtenerEstado();

    String json = "{\"ok\":true,\"estado\":\"" + estado.estado + "\"";

    if (estado.estado == "en_progreso" || estado.estado == "invitado") {
        if (estado.hayPregunta) {
            json += ",\"pregunta\":{";
            json += "\"idPregunta\":" + String(estado.pregunta.idPregunta) + ",";
            json += "\"textoPregunta\":\"" + estado.pregunta.textoPregunta + "\",";
            json += "\"numeroPregunta\":" + String(estado.pregunta.numeroPregunta) + ",";
            json += "\"totalPreguntas\":" + String(estado.pregunta.totalPreguntas) + ",";
            json += "\"opciones\":[";
            for (int i = 0; i < estado.pregunta.cantOpciones; i++) {
                if (i > 0) json += ",";
                json += "{\"idOpcion\":" + String(estado.pregunta.opciones[i].idOpcion) +
                        ",\"opcion\":\"" + estado.pregunta.opciones[i].opcion + "\"}";
            }
            json += "]}";
        }
    }

    if (estado.estado == "finalizado") {
        json += ",\"puntajeObtenido\":"    + String(estado.puntajeObtenido);
        json += ",\"puntajeParaAprobar\":" + String(estado.puntajeParaAprobar);
        json += ",\"aprobado\":"           + String(estado.aprobado ? "true" : "false");
        json += ",\"tiempoSegundos\":"     + String(estado.tiempoSegundos);
    }

    json += "}";
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// POST /api/alumno/responder
// Header: Authorization: Bearer <token>
// Body: { idPregunta, idOpcion }
// En modo invitado: { idPregunta: 0, idOpcion: <índice 0-3> }
// ---------------------------------------------------------------------------
static void handleAlumnoResponder(AsyncWebServerRequest* request, uint8_t* data,
                                    size_t len, size_t, size_t) {
    if (!verificarSesionAlumno(request)) return;

    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, data, len)) {
        enviarError(request, 400, "JSON inválido.");
        return;
    }

    int idPregunta = doc["idPregunta"] | 0;
    int idOpcion   = doc["idOpcion"]   | -1;

    if (idOpcion < 0) {
        enviarError(request, 400, "El campo 'idOpcion' es obligatorio.");
        return;
    }

    RespuestaResult result = RespuestaService::getInstance()
                             .responder(idPregunta, idOpcion);

    if (!result.ok) {
        enviarError(request, 400, result.mensaje);
        return;
    }

    String json = "{\"ok\":true";
    json += ",\"fueCorrecto\":"  + String(result.fueCorrecto ? "true" : "false");
    json += ",\"finalizo\":"     + String(result.finalizo    ? "true" : "false");
    if (result.mensaje.length() > 0) {
        json += ",\"mensaje\":\"" + result.mensaje + "\"";
    }
    json += "}";

    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// POST /api/alumno/heartbeat
// Header: Authorization: Bearer <token>
// ---------------------------------------------------------------------------
static void handleAlumnoHeartbeat(AsyncWebServerRequest* request, uint8_t* data,
                                    size_t len, size_t, size_t) {
    String token = extraerToken(request);
    if (!SessionManager::getInstance().heartbeatAlumno(token)) {
        enviarError(request, 401, "Sesión de alumno inválida o expirada.");
        return;
    }
    enviarOk(request, "ok");
}

// ---------------------------------------------------------------------------
// POST /api/invitado/pregunta
// Header: Authorization: Bearer <token> (sesión panel invitado)
// Body: { pregunta, opciones: ["...", "...", ...] }
// ---------------------------------------------------------------------------
static void handleInvitadoPregunta(AsyncWebServerRequest* request, uint8_t* data,
                                     size_t len, size_t, size_t) {
    if (!verificarSesionPanel(request, "invitado")) return;

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, data, len)) {
        enviarError(request, 400, "JSON inválido.");
        return;
    }

    PreguntaInvitado pregunta;
    pregunta.textoOpregunta = doc["pregunta"] | "";

    if (pregunta.textoOpregunta.length() == 0) {
        enviarError(request, 400, "La pregunta no puede estar vacía.");
        return;
    }

    JsonArray opcsJson = doc["opciones"].as<JsonArray>();
    if (opcsJson.isNull()) {
        enviarError(request, 400, "El campo 'opciones' es obligatorio.");
        return;
    }

    pregunta.cantOpciones = 0;
    for (const char* opc : opcsJson) {
        if (pregunta.cantOpciones >= 4) break;
        pregunta.opciones[pregunta.cantOpciones++] = String(opc);
    }

    if (pregunta.cantOpciones < 2) {
        enviarError(request, 400, "Se necesitan al menos 2 opciones.");
        return;
    }

    pregunta.cargada = true;
    SessionManager::getInstance().guardarPreguntaInvitado(pregunta);

    enviarOk(request, "Pregunta guardada.");
}

// ---------------------------------------------------------------------------
// Registro de rutas
// ---------------------------------------------------------------------------
void registrarRespuestaController(AsyncWebServer& server) {
    server.on("/api/alumno/iniciar", HTTP_POST,
              [](AsyncWebServerRequest* r){}, nullptr, handleAlumnoIniciar);

    server.on("/api/alumno/estado", HTTP_GET, handleAlumnoEstado);

    server.on("/api/alumno/responder", HTTP_POST,
              [](AsyncWebServerRequest* r){}, nullptr, handleAlumnoResponder);

    server.on("/api/alumno/heartbeat", HTTP_POST,
              [](AsyncWebServerRequest* r){}, nullptr, handleAlumnoHeartbeat);

    server.on("/api/invitado/pregunta", HTTP_POST,
              [](AsyncWebServerRequest* r){}, nullptr, handleInvitadoPregunta);
}
