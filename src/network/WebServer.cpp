#include "WebServer.h"
#include <LittleFS.h>
#include <SD.h>
#include <memory>
 
#include "controllers/AuthController.h"
#include "controllers/UsuarioController.h"
#include "controllers/CuestionarioController.h"
#include "controllers/RespuestaController.h"
 
AsyncWebServer server(80);
 
void initWebServer()
{
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
        String ip = request->client()->remoteIP().toString();
        Serial.printf("[OpenCV][%s] Pedido recibido (heap libre: %u)\n",
                      ip.c_str(), ESP.getFreeHeap());
 
        if (!SD.exists("/opencv.js.gz"))
        {
            Serial.printf("[OpenCV][%s] ERROR: opencv.js.gz no encontrado en la SD.\n", ip.c_str());
            request->send(404, "text/plain", "opencv.js no encontrado en SD.");
            return;
        }
 
        struct EstadoDescarga {
            File          file;
            unsigned long inicioMs   = 0;
            size_t        enviado    = 0;
            size_t        totalBytes = 0;
            int           chunkCount = 0;
            String        ip;
        };
 
        auto estado = std::make_shared<EstadoDescarga>();
        estado->ip = ip;
 
        // Tamaño total, solo para poder loguear el porcentaje de avance.
        File probe = SD.open("/opencv.js.gz", FILE_READ);
        if (probe) {
            estado->totalBytes = probe.size();
            probe.close();
        }
        Serial.printf("[OpenCV][%s] Tamaño del archivo: %u bytes\n",
                      ip.c_str(), (unsigned)estado->totalBytes);
 
        AsyncWebServerResponse *response = request->beginChunkedResponse(
            "text/javascript",
            [estado](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
 
                if (index == 0) {
                    estado->file    = SD.open("/opencv.js.gz", FILE_READ);
                    estado->inicioMs = millis();
                    Serial.printf("[OpenCV][%s] Archivo abierto, arranca el envío.\n",
                                  estado->ip.c_str());
                }
 
                if (!estado->file) {
                    Serial.printf("[OpenCV][%s] ERROR: el archivo se perdió o es inválido.\n",
                                  estado->ip.c_str());
                    return 0;
                }
 
                size_t chunk = (maxLen > 2048) ? 2048 : maxLen;
                size_t len   = estado->file.read(buffer, chunk);
                estado->enviado += len;
                estado->chunkCount++;
 
                if (estado->chunkCount % 20 == 0 || len == 0) {
                    int pct = estado->totalBytes > 0
                              ? (int)((estado->enviado * 100UL) / estado->totalBytes)
                              : -1;
                    Serial.printf("[OpenCV][%s] chunk #%d | %u/%u bytes (%d%%) | heap libre: %u\n",
                                  estado->ip.c_str(), estado->chunkCount,
                                  (unsigned)estado->enviado, (unsigned)estado->totalBytes,
                                  pct, ESP.getFreeHeap());
                }
 
                if (len == 0) {
                    estado->file.close();
                    unsigned long ms = millis() - estado->inicioMs;
                    float kbps = ms > 0
                                 ? (estado->enviado / 1024.0f) / (ms / 1000.0f)
                                 : 0.0f;
                    Serial.printf("[OpenCV][%s] Descarga terminada: %u bytes en %lu ms (%.1f KB/s)\n",
                                  estado->ip.c_str(), (unsigned)estado->enviado, ms, kbps);
                }
 
                return len;
            });
 
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "max-age=31536000");
        response->addHeader("Connection", "close");
 
        request->onDisconnect([estado]() {
            if (estado->enviado < estado->totalBytes) {
                Serial.printf("[OpenCV][%s] Conexión cerrada ANTES de terminar: %u/%u bytes.\n",
                              estado->ip.c_str(), (unsigned)estado->enviado, (unsigned)estado->totalBytes);
            }
        });
 
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