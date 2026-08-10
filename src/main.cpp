#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "esp_heap_caps.h"
#include "esp_bt.h"

#include "config/NetworkConfig.h"
#include "config/ConfigManager.h"
#include "storage/SDManager.h"
#include "storage/database/DatabaseManager.h"
#include "network/WiFiAP.h"
#include "network/WebServer.h"
#include "session/SessionManager.h"
#include "services/RespuestaService.h"  
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

// ---------------------------------------------------------------------------
// Helper de diagnóstico: heap total libre + bloque contiguo más grande.
// El total libre puede engañar; lo que de verdad importa para SQLite/JSON
// es cuánto hay disponible como UN SOLO bloque.
// ---------------------------------------------------------------------------
static void logHeap(const char* etiqueta) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    uint32_t freeHeapArduino = ESP.getFreeHeap();
    Serial.printf("[BOOT-HEAP] %-32s caps_libre=%7u  max_block=%7u  |  ESP.getFreeHeap()=%7u\n",
                  etiqueta, info.total_free_bytes, info.largest_free_block, freeHeapArduino);
}

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

    logHeap("Arranque (post pines)");

    // -------------------------------------------------------------------
    // Bluetooth: si el build lo trae linkeado pero nunca se usa, el
    // controlador BT/BLE puede estar reservando memoria del heap general
    // sin que se note a simple vista. Lo liberamos explícitamente.
    // -------------------------------------------------------------------
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
    logHeap("Post liberar memoria BT");

    // 1. LittleFS
    if (!LittleFS.begin(true))
    {
        Serial.println("[Main] Error: no se pudo montar LittleFS.");
        return;
    }
    Serial.println("[Main] LittleFS montado.");
    logHeap("Post LittleFS.begin()");

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
    logHeap("Post ConfigManager (claves)");

    // 3. INICIALIZAR LA PANTALLA TÁCTIL (CYD) — REACTIVADA para medir su costo real
    logHeap("PRE ui_init()");
    ui_init(esPrimerInicio);
    logHeap("POST ui_init()");
    Serial.println("[Main] Pantalla inicializada.");

    // 4. SD
    logHeap("PRE initSD()");
    if (!initSD())
    {
        Serial.println("[Main] Error: no se pudo montar la SD.");
    }
    logHeap("POST initSD()");

    // 5. Base de datos
    logHeap("PRE DatabaseManager.begin()");
    if (!DatabaseManager::getInstance().begin("/sd/levi.db"))
    {
        Serial.println("[Main] Error: no se pudo inicializar la base de datos.");
    }
    logHeap("POST DatabaseManager.begin()");

    // 6. WiFi Access Point
    logHeap("PRE initWiFiAP()");
    initWiFiAP();
    logHeap("POST initWiFiAP()");

    // 7. Servidor web
    logHeap("PRE initWebServer()");
    initWebServer();
    logHeap("POST initWebServer()");

    logHeap("FIN DE SETUP");
    Serial.println("[Main] Sistema completamente listo.");
}

void loop()
{
    unsigned long ahora = millis();

    static int _ticksContados = 0;
    static bool _checkpointHecho = false;

    if (ahora - _ultimoTick >= TICK_INTERVAL_MS)
    {
        _ultimoTick = ahora;
        SessionManager::getInstance().tick();
        int clientesConectados = WiFi.softAPgetStationNum();
        ui_update_dispositivos(clientesConectados);

        EstadoExamenResumen resumenExamen = RespuestaService::getInstance().obtenerResumenCacheado();
        ui_update_examen(resumenExamen.estado, resumenExamen.tituloCuestionario, resumenExamen.numeroPregunta, resumenExamen.totalPreguntas);

        Serial.printf(
            "[Monitor] Heap libre: %u | Heap min historico: %u | Stack loopTask libre: %u\n",
            ESP.getFreeHeap(),
            ESP.getMinFreeHeap(),
            uxTaskGetStackHighWaterMark(NULL));

        _ticksContados++;
        // A los 15 segundos de loop (15 ticks), la pantalla ya debería estar
        // completamente dibujada y estabilizada. Este es el número real y
        // comparable de "costo total de la interfaz activa", no el de
        // ui_init() solo, que corta a mitad de camino.
        if (_ticksContados == 15 && !_checkpointHecho) {
            _checkpointHecho = true;
            logHeap(">>> CHECKPOINT ESTABLE (15s de loop) <<<");
        }
    }

    // Tareas de la interfaz gráfica
    unsigned long t0 = micros();
    ui_loop();
    unsigned long dt = micros() - t0;
    if (dt > 15000)
    {
        Serial.printf("[Monitor] ui_loop() tardo %lu us (heap libre: %u)\n", dt, ESP.getFreeHeap());
    }
}