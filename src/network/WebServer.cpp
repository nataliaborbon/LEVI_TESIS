#include "WebServer.h"
#include <LittleFS.h>
#include <SD.h>

#include "../controllers/AuthController.h"
#include "../controllers/UsuarioController.h"
#include "../controllers/CuestionarioController.h"
#include "../controllers/RespuestaController.h"

AsyncWebServer server(80);

void initWebServer()
{
    // -----------------------------------------------------------------------
    // CORS preflight
    // -----------------------------------------------------------------------
    server.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
              {
        AsyncWebServerResponse* response = request->beginResponse(200);
        response->addHeader("Access-Control-Allow-Origin",  "*");
        response->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type,Authorization");
        request->send(response); });

    // -----------------------------------------------------------------------
    // API
    // -----------------------------------------------------------------------
    registrarAuthController(server);
    registrarUsuarioController(server);
    registrarCuestionarioController(server);
    registrarRespuestaController(server);

    // -----------------------------------------------------------------------
    // opencv.js desde SD
    // -----------------------------------------------------------------------
    server.on("/opencv.js", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (!SD.exists("/opencv.js"))
        {
            request->send(404, "text/plain", "opencv.js no encontrado en SD.");
            return;
        }

        AsyncWebServerResponse* response =
            request->beginResponse(SD, "/opencv.js", "text/javascript");

        response->addHeader("Cache-Control", "max-age=31536000");

        request->send(response); });

    // -----------------------------------------------------------------------
    // Redirección IP -> levi.local
    // -----------------------------------------------------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String host = request->host();

        if (host != "levi.local")
        {
            request->redirect("http://levi.local");
            return;
        }

        request->send(LittleFS, "/index.html", "text/html"); });

    // -----------------------------------------------------------------------
    // React SPA
    // -----------------------------------------------------------------------
    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("index.html");

    // -----------------------------------------------------------------------
    // 404 - Catch-All para React (Single Page Application)
    // -----------------------------------------------------------------------
    server.onNotFound([](AsyncWebServerRequest *request)
    {
        String url = request->url();
        if (url.startsWith("/api/"))
        {
            request->send(404, "application/json", "{\"ok\":false,\"mensaje\":\"Endpoint de la API no encontrado.\"}");
            return;
        }
        request->send(LittleFS, "/index.html", "text/html"); 
    });

    server.begin();

    Serial.println("[WebServer] Servidor iniciado en el puerto 80.");
}