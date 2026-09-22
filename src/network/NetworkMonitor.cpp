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

void networkMonitor_init()
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

void networkMonitor_loop()
{
    _cantidadDispositivos = WiFi.softAPgetStationNum();
}

int networkMonitor_getCantidadDispositivos() { return _cantidadDispositivos; }
bool networkMonitor_isCamaraConectada() { return _camaraConectada; }