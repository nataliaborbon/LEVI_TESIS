#include "web_server.h"
#include "SD.h"

// Instancia global del servidor en el puerto 80
AsyncWebServer server(80);

void initWebServer()
{
    // Ruta Principal (Index)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        Serial.println("📡 Despachando index.html");
        if (SD.exists("/index.html")) {
            request->send(SD, "/index.html", "text/html");
        } else {
            request->send(500, "text/plain", "ERROR FATAL: No se encuentra /index.html en la SD.");
        } });

    // Ruta de la Librería con Caché de 1 año
    server.on("/opencv.js", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        Serial.println("📡 Despachando OpenCV.js con Caché...");
        if (SD.exists("/opencv.js")) {
            AsyncWebServerResponse *response = request->beginResponse(SD, "/opencv.js", "text/javascript");
            response->addHeader("Cache-Control", "max-age=31536000"); 
            request->send(response);
        } else {
            request->send(404, "text/plain", "ERROR FATAL: No se encuentra /opencv.js en la SD.");
        } });

    server.begin();
    Serial.println("🚀 Servidor Web corriendo.");
}