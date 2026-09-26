#include "network/NetworkMonitor.h"
#include "config/NetworkConfig.h"

#include <WiFi.h>
#include "ping/ping_sock.h"
#include "lwip/inet.h"

static esp_ping_handle_t _pingCamara = nullptr;
static bool _camaraConectada = false;
static int _cantidadDispositivos = 0;

static void onPingSuccess(esp_ping_handle_t hdl, void *args)
{
    _camaraConectada = true;
}

static void onPingTimeout(esp_ping_handle_t hdl, void *args)
{
    _camaraConectada = false;
}

// Crea la sesión de ping y la arranca. Se separa de networkMonitor_init()
// para poder volver a llamarla cuando la pantalla se despierta, después
// de haber sido destruida por networkMonitor_detenerPingCamara().
static void crearYArrancarPing()
{
    esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
    cfg.target_addr.u_addr.ip4.addr = static_cast<uint32_t>(CAM_IP);
    cfg.target_addr.type = IPADDR_TYPE_V4;
    cfg.count = ESP_PING_COUNT_INFINITE;
    cfg.interval_ms = 2000;
    cfg.timeout_ms = 500;

    esp_ping_callbacks_t cbs = {};
    cbs.on_ping_success = onPingSuccess;
    cbs.on_ping_timeout = onPingTimeout;

    esp_ping_new_session(&cfg, &cbs, &_pingCamara);
    esp_ping_start(_pingCamara);
}

void networkMonitor_init()
{
    crearYArrancarPing();
}

void networkMonitor_loop()
{
    _cantidadDispositivos = WiFi.softAPgetStationNum();
}

// Se llama cuando la pantalla se suspende por inactividad: el ping sigue
// vivo en su propio task interno del stack de red aunque nadie llame a
// networkMonitor_loop(), así que hay que destruirlo explícitamente.
void networkMonitor_detenerPingCamara()
{
    if (_pingCamara == nullptr) return; // ya estaba detenido

    esp_ping_stop(_pingCamara);
    esp_ping_delete_session(_pingCamara);
    _pingCamara = nullptr;

    // Sin sesión de ping no tenemos forma de confirmar el estado real de
    // la cámara: mejor dejarlo en "desconectada" que mostrar un dato
    // viejo como si siguiera siendo válido.
    _camaraConectada = false;
}

// Se llama cuando la pantalla se despierta: recrea la sesión de ping
// destruida por networkMonitor_detenerPingCamara().
void networkMonitor_reanudarPingCamara()
{
    if (_pingCamara != nullptr) return; // ya está corriendo

    crearYArrancarPing();
}

int networkMonitor_getCantidadDispositivos() { return _cantidadDispositivos; }
bool networkMonitor_isCamaraConectada() { return _camaraConectada; }