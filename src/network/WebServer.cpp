#include "WebServer.h"
#include <LittleFS.h>
#include <SD.h>
#include <memory>
#include <esp_task_wdt.h>

#include "controllers/AuthController.h"
#include "controllers/UsuarioController.h"
#include "controllers/CuestionarioController.h"
#include "controllers/RespuestaController.h"

AsyncWebServer server(80);

void initWebServer()
{

    DefaultHeaders::Instance().addHeader("Connection", "close");
    // -----------------------------------------------------------------------
    // API
    // -----------------------------------------------------------------------
    registrarAuthController(server);
    registrarUsuarioController(server);
    registrarCuestionarioController(server);
    registrarRespuestaController(server);

   // -----------------------------------------------------------------------
    // opencv.js desde SD (Envío nativo gestionado por AsyncWebServer)
    // -----------------------------------------------------------------------
    server.on("/opencv.js", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String ip = request->client()->remoteIP().toString();
        Serial.printf("[OpenCV][%s] Pedido recibido (heap libre: %u)\n", 
                      ip.c_str(), ESP.getFreeHeap());

        if (!SD.exists("/opencv.js.gz"))
        {
            Serial.printf("[OpenCV][%s] ERROR: opencv.js.gz no encontrado en la SD.\n", ip.c_str());
            request->send(404, "text/plain", "opencv.js no encontrado en SD.");
            return;
        }

        // Delegamos todo el trabajo pesado de lectura, chunking e índices (retransmisiones)
        // a la función nativa de la librería, que es 100% a prueba de fallos de red.
        AsyncWebServerResponse *response = request->beginResponse(SD, "/opencv.js.gz", "text/javascript");
        
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "max-age=31536000");
        response->addHeader("Connection", "close");

        request->send(response);
    });

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
    // 404 - Catch-All para React (Single Page Application) + CORS preflight
    // -----------------------------------------------------------------------
    server.onNotFound([](AsyncWebServerRequest *request)
    {
        if (request->method() == HTTP_OPTIONS)
        {
            AsyncWebServerResponse* response = request->beginResponse(200);
            response->addHeader("Access-Control-Allow-Origin",  "*");
            response->addHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            response->addHeader("Access-Control-Allow-Headers", "Content-Type,Authorization");
            request->send(response);
            return;
        }

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