#include <Arduino.h>
#include "wifi_ap.h"
#include "sd_manager.h"
#include "web_server.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 1. Iniciar SD
    if (initSD()) {
        // 2. Grabar el HTML (con los filtros avanzados) en la memoria
        writeHtmlToSD();
    }

    // 3. Crear la red Wi-Fi
    initWiFiAP();

    // 4. Iniciar el Servidor Web
    initWebServer();
}

void loop() {
    // AsyncWebServer maneja todo en segundo plano. No se requiere código en el loop.
}