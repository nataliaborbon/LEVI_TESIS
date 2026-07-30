#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <Preferences.h> 

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

static constexpr unsigned long TICK_INTERVAL_MS = 1000UL;
static unsigned long _ultimoTick = 0;

const int LED_ROJO = 4;
const int LED_VERDE = 16;
const int LED_AZUL = 17;

// Variable global para que el loop sepa si estamos en modo configuración
bool g_esPrimerInicio = false; 

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

    // --- BLOQUE DE BORRADO DE PRUEBA (deshabilitado) ---
    // El namespace real que usa ConfigManager es "levi_cfg" (ver ConfigManager.h),
    // no "config". Si en algún momento necesitás forzar el primer arranque para
    // pruebas, descomentá estas 3 líneas, flasheá UNA vez, y volvé a comentarlas
    // antes de la versión final (o mejor: agregá un método
    // ConfigManager::resetearClaves() y llamalo desde acá).
    //
    // Preferences pref;
    // pref.begin("levi_cfg", false);
    // pref.clear();
    // pref.end();

    // 2. Revisar Configuración de Claves
    if (!ConfigManager::getInstance().clavesConfiguradas())
    {
        Serial.println("[Main] Claves maestras NO configuradas. Se requiere configuracion inicial.");
        g_esPrimerInicio = true;
    }
    else
    {
        Serial.println("[Main] Claves maestras encontradas en memoria.");
    }

    // 3. INICIALIZAR LA PANTALLA TÁCTIL (CYD)
    ui_init(g_esPrimerInicio);
    Serial.println("[Main] Pantalla inicializada.");

    // 4. EL FRENO DE MANO
    if (g_esPrimerInicio)
    {
        Serial.println("[Main] MODO CONFIGURACIÓN: Deteniendo arranque de periféricos.");
        // Cortamos el setup acá. 
        return; 
    }

    // =========================================================================
    // INICIO NORMAL
    // =========================================================================

    // 5. SD
    if (!initSD())
    {
        Serial.println("[Main] Error: no se pudo montar la SD.");
    }

    // 6. Base de datos
    if (!DatabaseManager::getInstance().begin("/sd/levi.db"))
    {
        Serial.println("[Main] Error: no se pudo inicializar la base de datos.");
    }

    // 7. WiFi Access Point
    initWiFiAP();

    // 8. Servidor web
    initWebServer();

    Serial.println("[Main] Sistema completamente listo.");
}

void loop()
{
    unsigned long ahora = millis();

    // Solo ejecutamos la lógica de red si NO estamos en la pantalla de primer inicio
    if (!g_esPrimerInicio)
    {
        if (ahora - _ultimoTick >= TICK_INTERVAL_MS)
        {
            _ultimoTick = ahora;
            SessionManager::getInstance().tick();
            int clientesConectados = WiFi.softAPgetStationNum();
            ui_update_dispositivos(clientesConectados);

            EstadoExamenResumen resumenExamen = RespuestaService::getInstance().obtenerResumenCacheado();
            ui_update_examen(resumenExamen.estado, resumenExamen.tituloCuestionario, resumenExamen.numeroPregunta, resumenExamen.totalPreguntas);
        }
    }

    // Tareas de la interfaz gráfica (esto debe correr SIEMPRE para que funcione el touch)
    ui_loop();
}