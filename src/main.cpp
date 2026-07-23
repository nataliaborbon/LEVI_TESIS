#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "config/NetworkConfig.h"
#include "config/ConfigManager.h"
#include "storage/SDManager.h"
#include "storage/database/DatabaseManager.h"
#include "network/WiFiAP.h"
#include "network/WebServer.h"
#include "session/SessionManager.h"
#include "ui/screens.h"
#include "ui/ui_manager.h"

/**
 * @file main.cpp
 * @brief Punto de entrada del firmware del CYD.
 */
static constexpr unsigned long TICK_INTERVAL_MS = 1000UL;
static unsigned long _ultimoTick = 0;

const int LED_ROJO = 4;
const int LED_VERDE = 16;
const int LED_AZUL = 17;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_ROJO, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_AZUL, OUTPUT);

    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_AZUL, HIGH);

    // 1. LittleFS
    if (!LittleFS.begin(true))
    {
        Serial.println("[Main] Error: no se pudo montar LittleFS.");
        return;
    }
    Serial.println("[Main] LittleFS montado.");
    Serial.printf("[Monitor] Heap libre tras LittleFS: %u\n", ESP.getFreeHeap());

    // 2. Claves maestras (LÓGICA PARA LA INTERFAZ)
    bool esPrimerInicio = false;
    if (!ConfigManager::getInstance().clavesConfiguradas())
    {
        Serial.println("[Main] Claves maestras NO configuradas. Se requiere configuracion inicial.");
        esPrimerInicio = true;
    }
    else
    {
        Serial.println("[Main] Claves maestras encontradas en memoria.");
    }

    // 3. INICIALIZAR LA PANTALLA TÁCTIL (CYD)
    Serial.printf("[Monitor] Heap libre ANTES de ui_init: %u\n", ESP.getFreeHeap());
    ui_init(esPrimerInicio);
    Serial.printf("[Monitor] Heap libre DESPUES de ui_init: %u\n", ESP.getFreeHeap());
    Serial.println("[Main] Pantalla inicializada.");

    // 4. SD
    if (!initSD())
    {
        Serial.println("[Main] Error: no se pudo montar la SD.");
    }
    Serial.printf("[Monitor] Heap libre DESPUES de initSD: %u\n", ESP.getFreeHeap());

    // 5. Base de datos
    if (!DatabaseManager::getInstance().begin("/sd/levi.db"))
    {
        Serial.println("[Main] Error: no se pudo inicializar la base de datos.");
    }
    Serial.printf("[Monitor] Heap libre DESPUES de DB: %u\n", ESP.getFreeHeap());

    // 6. WiFi Access Point
    initWiFiAP();

    // 7. Servidor web
    initWebServer();

    Serial.printf("[Monitor] Heap libre AL FINAL DE SETUP: %u\n", ESP.getFreeHeap());
    Serial.println("[Main] Sistema completamente listo.");
}

void loop()
{
    unsigned long ahora = millis();

    if (ahora - _ultimoTick >= TICK_INTERVAL_MS)
    {
        _ultimoTick = ahora;
        SessionManager::getInstance().tick();
        int clientesConectados = WiFi.softAPgetStationNum();
        ui_update_dispositivos(clientesConectados);
        //Serial.printf(
        //    "[Monitor] Heap libre: %u | Heap min historico: %u | Stack loopTask libre: %u\n",
        //    ESP.getFreeHeap(),
        //    ESP.getMinFreeHeap(),
        //    uxTaskGetStackHighWaterMark(NULL));
    }

    // Tareas de la interfaz gráfica
    unsigned long t0 = micros();
    ui_loop();
    unsigned long dt = micros() - t0;
    if (dt > 15000)
    {
        //Serial.printf("[Monitor] ui_loop() tardo %lu us (heap libre: %u)\n", dt, ESP.getFreeHeap());
    }
}