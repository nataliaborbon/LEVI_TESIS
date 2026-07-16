#include "controllers/CuestionarioController.h"
#include "middleware/Middleware.h"
#include "services/CuestionarioService.h"
#include "session/SessionManager.h"
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// GET /api/cuestionarios (PLURAL - Lista Todos)
// ---------------------------------------------------------------------------
static void handleListar(AsyncWebServerRequest* request) {
    if (!verificarSesionPanel(request)) return;
    const SesionPanel& sesion = SessionManager::getInstance().getSesionPanel();
    if (sesion.rol == "invitado") { enviarError(request, 403, "Sin permisos."); return; }

    String json = "{\"ok\":true,\"cuestionarios\":[";
    if (sesion.rol == "profesor") {
        CuestionarioResumenProfesor buffer[50];
        int cant = CuestionarioService::getInstance().listarProfesor(sesion.idUsuario, buffer, 50);
        for (int i = 0; i < cant; i++) {
            if (i > 0) json += ",";
            json += "{\"idCuestionario\":" + String(buffer[i].idCuestionario) + ",";
            json += "\"titulo\":\"" + buffer[i].titulo + "\",";
            json += "\"estado\":\"" + buffer[i].estado + "\",";
            json += "\"puntajeObtenido\":" + String(buffer[i].puntajeObtenido) + ",";
            json += "\"puntajeParaAprobar\":" + String(buffer[i].puntajeParaAprobar) + ",";
            json += "\"cantPreguntas\":" + String(buffer[i].cantPreguntas) + ",";
            json += "\"aprobado\":" + String(buffer[i].aprobado ? "true" : "false") + "}";
        }
    } else {
        CuestionarioResumenTutor buffer[50];
        int cant = CuestionarioService::getInstance().listarTutor(buffer, 50);
        for (int i = 0; i < cant; i++) {
            if (i > 0) json += ",";
            json += "{\"idCuestionario\":" + String(buffer[i].idCuestionario) + ",";
            json += "\"titulo\":\"" + buffer[i].titulo + "\",";
            json += "\"estado\":\"" + buffer[i].estado + "\",";
            json += "\"puntajeObtenido\":" + String(buffer[i].puntajeObtenido) + ",";
            json += "\"puntajeParaAprobar\":" + String(buffer[i].puntajeParaAprobar) + ",";
            json += "\"cantPreguntas\":" + String(buffer[i].cantPreguntas) + ",";
            json += "\"aprobado\":" + String(buffer[i].aprobado ? "true" : "false") + ",";
            json += "\"materia\":\"" + buffer[i].materia + "\"}";
        }
    }
    json += "]}";
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// GET /api/cuestionario?id=X (SINGULAR - Obtener Uno)
// ---------------------------------------------------------------------------
static void handleObtener(AsyncWebServerRequest* request) {
    if (!verificarSesionPanel(request)) return;
    if (!request->hasParam("id")) { enviarError(request, 400, "Falta ID"); return; }
    int idCuestionario = request->getParam("id")->value().toInt();

    Cuestionario c;
    PreguntaCompleta* preguntas = new PreguntaCompleta[20]; 
    int cantPreguntas = 0;

    if (!CuestionarioService::getInstance().obtenerCompleto(idCuestionario, c, preguntas, cantPreguntas)) {
        delete[] preguntas; 
        enviarError(request, 404, "No encontrado.");
        return;
    }

    String json; json.reserve(2048);
    json += "{\"ok\":true,";
    auto escapeJson = [](String str) {
        str.replace("\\", "\\\\"); str.replace("\"", "\\\"");
        str.replace("\n", "\\n"); str.replace("\r", ""); return str;
    };

    json += "\"titulo\":\"" + escapeJson(c.titulo) + "\",";
    json += "\"puntajeParaAprobar\":" + String(c.puntajeParaAprobar) + ",";
    json += "\"preguntas\":[";

    for (int i = 0; i < cantPreguntas; i++) {
        if (i > 0) json += ",";
        json += "{\"pregunta\":\"" + escapeJson(preguntas[i].pregunta.pregunta) + "\",";
        json += "\"puntajeCorrecta\":" + String(preguntas[i].pregunta.puntajeCorrecta) + ",";
        json += "\"puntajeIncorrecta\":" + String(preguntas[i].pregunta.puntajeIncorrecta) + ",";
        json += "\"opciones\":[";
        for (int j = 0; j < preguntas[i].cantOpciones; j++) {
            if (j > 0) json += ",";
            json += "{\"opcion\":\"" + escapeJson(preguntas[i].opciones[j].opcion) + "\",";
            json += "\"esCorrecta\":" + String(preguntas[i].opciones[j].esCorrecta ? "true" : "false") + "}";
        }
        json += "]}";
    }
    json += "]}";
    delete[] preguntas; 
    enviarJSON(request, 200, json);
}

// ---------------------------------------------------------------------------
// POST /api/cuestionarios (Crear Nuevo)
// ---------------------------------------------------------------------------
static void handleCrear(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
    if (!verificarSesionPanel(request, "profesor")) return;
    const SesionPanel& sesion = SessionManager::getInstance().getSesionPanel();

    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, data, len)) { enviarError(request, 400, "JSON inválido."); return; }

    Cuestionario c;
    c.idUsuario = sesion.idUsuario;
    c.titulo = doc["titulo"] | "";
    c.puntajeParaAprobar = doc["puntajeParaAprobar"] | 0.0f;

    JsonArray preguntasJson = doc["preguntas"].as<JsonArray>();
    if (preguntasJson.isNull()) { enviarError(request, 400, "Falta 'preguntas'."); return; }

    int cantPreguntas = preguntasJson.size();
    PreguntaCompleta preguntas[20];
    int idx = 0;

    for (JsonObject pj : preguntasJson) {
        if (idx >= 20) break;
        preguntas[idx].pregunta.pregunta = pj["pregunta"] | "";
        preguntas[idx].pregunta.puntajeCorrecta = pj["puntajeCorrecta"] | 0.0f;
        preguntas[idx].pregunta.puntajeIncorrecta = pj["puntajeIncorrecta"] | 0.0f;
        JsonArray opcsJson = pj["opciones"].as<JsonArray>();
        preguntas[idx].cantOpciones = 0;
        for (JsonObject oj : opcsJson) {
            if (preguntas[idx].cantOpciones >= 4) break;
            int j = preguntas[idx].cantOpciones++;
            preguntas[idx].opciones[j].opcion = oj["opcion"] | "";
            preguntas[idx].opciones[j].esCorrecta = oj["esCorrecta"] | false;
        }
        idx++;
    }

    CuestionarioResult result = CuestionarioService::getInstance().crear(c, preguntas, cantPreguntas);
    if (!result.ok) { enviarError(request, 400, result.mensaje); return; }
    String json = "{\"ok\":true,\"idCuestionario\":" + String(result.id) + "}";
    enviarJSON(request, 201, json);
}

// ---------------------------------------------------------------------------
// PUT /api/cuestionario?id=X (Editar Existente)
// ---------------------------------------------------------------------------
static void handleEditar(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
    if (!verificarSesionPanel(request, "profesor")) return;
    const SesionPanel& sesion = SessionManager::getInstance().getSesionPanel();

    if (!request->hasParam("id")) { enviarError(request, 400, "Falta ID"); return; }
    int idCuestionario = request->getParam("id")->value().toInt();

    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, data, len)) { enviarError(request, 400, "JSON inválido."); return; }

    Cuestionario c;
    c.idCuestionario = idCuestionario;
    c.idUsuario = sesion.idUsuario;
    c.titulo = doc["titulo"] | "";
    c.puntajeParaAprobar = doc["puntajeParaAprobar"] | 0.0f;

    JsonArray preguntasJson = doc["preguntas"].as<JsonArray>();
    int cantPreguntas = preguntasJson.size();
    PreguntaCompleta preguntas[20];
    int idx = 0;

    for (JsonObject pj : preguntasJson) {
        if (idx >= 20) break;
        preguntas[idx].pregunta.pregunta = pj["pregunta"] | "";
        preguntas[idx].pregunta.puntajeCorrecta = pj["puntajeCorrecta"] | 0.0f;
        preguntas[idx].pregunta.puntajeIncorrecta = pj["puntajeIncorrecta"] | 0.0f;
        JsonArray opcsJson = pj["opciones"].as<JsonArray>();
        preguntas[idx].cantOpciones = 0;
        for (JsonObject oj : opcsJson) {
            if (preguntas[idx].cantOpciones >= 4) break;
            int j = preguntas[idx].cantOpciones++;
            preguntas[idx].opciones[j].opcion = oj["opcion"] | "";
            preguntas[idx].opciones[j].esCorrecta = oj["esCorrecta"] | false;
        }
        idx++;
    }

    CuestionarioResult result = CuestionarioService::getInstance().editar(c, preguntas, cantPreguntas);
    if (!result.ok) { enviarError(request, 400, result.mensaje); return; }
    enviarOk(request, "Cuestionario actualizado.");
}

// ---------------------------------------------------------------------------
// Acciones Básicas y Registro
// ---------------------------------------------------------------------------
static void handleEliminar(AsyncWebServerRequest* r, uint8_t*, size_t, size_t, size_t) {
    if (!verificarSesionPanel(r, "profesor")) return;
    if (!r->hasParam("id")) { enviarError(r, 400, "Falta ID"); return; }
    CuestionarioResult result = CuestionarioService::getInstance().eliminar(r->getParam("id")->value().toInt(), SessionManager::getInstance().getSesionPanel().idUsuario);
    if (!result.ok) enviarError(r, 400, result.mensaje); else enviarOk(r, "Eliminado.");
}

static void handleIniciar(AsyncWebServerRequest* r, uint8_t*, size_t, size_t, size_t) {
    if (!r->hasParam("id")) { enviarError(r, 400, "Falta ID"); return; }
    CuestionarioResult result = CuestionarioService::getInstance().iniciar(r->getParam("id")->value().toInt(), SessionManager::getInstance().getSesionPanel().idUsuario);
    if (!result.ok) enviarError(r, 400, result.mensaje); else enviarOk(r, "Iniciado.");
}

static void handlePausar(AsyncWebServerRequest* r, uint8_t*, size_t, size_t, size_t) {
    if (!r->hasParam("id")) { enviarError(r, 400, "Falta ID"); return; }
    CuestionarioResult result = CuestionarioService::getInstance().pausar(r->getParam("id")->value().toInt(), SessionManager::getInstance().getSesionPanel().idUsuario);
    if (!result.ok) enviarError(r, 400, result.mensaje); else enviarOk(r, "Pausado.");
}

static void handleReanudar(AsyncWebServerRequest* r, uint8_t*, size_t, size_t, size_t) {
    if (!r->hasParam("id")) { enviarError(r, 400, "Falta ID"); return; }
    CuestionarioResult result = CuestionarioService::getInstance().reanudar(r->getParam("id")->value().toInt(), SessionManager::getInstance().getSesionPanel().idUsuario);
    if (!result.ok) enviarError(r, 400, result.mensaje); else enviarOk(r, "Reanudado.");
}

static void handleFinalizar(AsyncWebServerRequest* r, uint8_t*, size_t, size_t, size_t) {
    if (!r->hasParam("id")) { enviarError(r, 400, "Falta ID"); return; }
    CuestionarioResult result = CuestionarioService::getInstance().finalizar(
        r->getParam("id")->value().toInt(),
        SessionManager::getInstance().getSesionPanel().idUsuario
    );
    
    if (!result.ok) enviarError(r, 400, result.mensaje); else enviarOk(r, "Finalizado.");
}

static void handleRevision(AsyncWebServerRequest* r) {
    if (!verificarSesionPanel(r)) return;
    if (!r->hasParam("id")) { enviarError(r, 400, "Falta ID"); return; }
    PreguntaRevision buffer[20];
    int cant = CuestionarioService::getInstance().obtenerRevision(r->getParam("id")->value().toInt(), buffer, 20);
    if (cant < 0) { enviarError(r, 404, "No encontrado"); return; }
    
    String json = "{\"ok\":true,\"preguntas\":[";
    for (int i = 0; i < cant; i++) {
        if (i > 0) json += ",";
        json += "{\"idPregunta\":" + String(buffer[i].idPregunta) + ",\"pregunta\":\"" + buffer[i].textoPregunta + "\",\"puntajeCorrecta\":" + String(buffer[i].puntajeCorrecta) + ",\"puntajeIncorrecta\":" + String(buffer[i].puntajeIncorrecta) + ",\"opcionElegida\":\"" + buffer[i].opcionElegida + "\",\"opcionCorrecta\":\"" + buffer[i].opcionCorrecta + "\",\"fueCorrecto\":" + String(buffer[i].fueCorrecto ? "true" : "false") + "}";
    }
    json += "]}";
    enviarJSON(r, 200, json);
}

void registrarCuestionarioController(AsyncWebServer& server) {
    server.on("/api/cuestionario/revision", HTTP_GET, handleRevision);  // ← subir esta
    server.on("/api/cuestionario/iniciar",  HTTP_PATCH, [](AsyncWebServerRequest* r){}, nullptr, handleIniciar);
    server.on("/api/cuestionario/pausar",   HTTP_PATCH, [](AsyncWebServerRequest* r){}, nullptr, handlePausar);
    server.on("/api/cuestionario/reanudar", HTTP_PATCH, [](AsyncWebServerRequest* r){}, nullptr, handleReanudar);
    server.on("/api/cuestionario/finalizar",HTTP_PATCH, [](AsyncWebServerRequest* r){}, nullptr, handleFinalizar);

    server.on("/api/cuestionarios", HTTP_GET, handleListar);
    server.on("/api/cuestionarios", HTTP_POST, [](AsyncWebServerRequest* r){}, nullptr, handleCrear);
    server.on("/api/cuestionario", HTTP_GET, handleObtener);
    server.on("/api/cuestionario", HTTP_PUT,    [](AsyncWebServerRequest* r){}, nullptr, handleEditar);
    server.on("/api/cuestionario", HTTP_DELETE, [](AsyncWebServerRequest* r) {
        if (!verificarSesionPanel(r, "profesor")) return;
        if (!r->hasParam("id")) { enviarError(r, 400, "Falta ID"); return; }
        CuestionarioResult result = CuestionarioService::getInstance().eliminar(
            r->getParam("id")->value().toInt(),
            SessionManager::getInstance().getSesionPanel().idUsuario
        );
        if (!result.ok) enviarError(r, 400, result.mensaje); else enviarOk(r, "Eliminado.");
    }); 
}
