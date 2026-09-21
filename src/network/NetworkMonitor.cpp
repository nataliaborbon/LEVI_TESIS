#include "network/NetworkMonitor.h"
#include "config/NetworkConfig.h"

#include "esp_wifi.h"
#include <WiFi.h>

#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "dhcpserver/dhcpserver.h"

static const uint32_t INTERVALO_MS = 2000;

struct EstadoEstacion
{
    uint8_t mac[6];
    bool esperandoRespuesta = false;
};

static constexpr int MAX_ESTACIONES = 10;
static EstadoEstacion _estados[MAX_ESTACIONES];
static int _cantidadEstadosUsados = 0;

static uint32_t _ultimoChequeo = 0;
static int _cantidadDispositivos = 0;
static bool _camaraConectada = false;

static struct netif *buscarNetifDelAP()
{
    struct netif *iter;
    NETIF_FOREACH(iter)
    {
        if (iter->ip_addr.u_addr.ip4.addr == static_cast<uint32_t>(CYD_IP))
            return iter;
    }
    return nullptr;
}

static EstadoEstacion *obtenerEstado(const uint8_t *mac)
{
    for (int i = 0; i < _cantidadEstadosUsados; i++)
    {
        if (memcmp(_estados[i].mac, mac, 6) == 0)
            return &_estados[i];
    }
    if (_cantidadEstadosUsados < MAX_ESTACIONES)
    {
        EstadoEstacion *nuevo = &_estados[_cantidadEstadosUsados++];
        memcpy(nuevo->mac, mac, 6);
        nuevo->esperandoRespuesta = false;
        return nuevo;
    }
    return nullptr;
}

static void limpiarEstadosDeMacsAusentes(const wifi_sta_list_t &lista)
{
    int nuevoTope = 0;
    for (int i = 0; i < _cantidadEstadosUsados; i++)
    {
        bool sigueEnLista = false;
        for (int j = 0; j < lista.num; j++)
        {
            if (memcmp(_estados[i].mac, lista.sta[j].mac, 6) == 0)
            {
                sigueEnLista = true;
                break;
            }
        }
        if (sigueEnLista)
            _estados[nuevoTope++] = _estados[i];
    }
    _cantidadEstadosUsados = nuevoTope;
}

void networkMonitor_loop()
{
    uint32_t ahora = millis();
    if (ahora - _ultimoChequeo < INTERVALO_MS)
        return;
    _ultimoChequeo = ahora;

    wifi_sta_list_t listaWifi;
    if (esp_wifi_ap_get_sta_list(&listaWifi) != ESP_OK)
        return;

    limpiarEstadosDeMacsAusentes(listaWifi);

    struct netif *netifAP = buscarNetifDelAP();
    if (netifAP == nullptr)
        return;

    ip4_addr_t ipCamara;
    ipCamara.addr = static_cast<uint32_t>(CAM_IP);

    int cantidadRespondieron = 0;

    for (int i = 0; i < listaWifi.num; i++)
    {
        wifi_sta_info_t &staWifi = listaWifi.sta[i];

        ip4_addr_t ipEstacion;
        if (!dhcp_search_ip_on_mac(staWifi.mac, &ipEstacion))
            continue; // sin lease DHCP todavia (en pleno handshake): no tocar

        EstadoEstacion *estado = obtenerEstado(staWifi.mac);
        if (estado == nullptr)
            continue;

        bool esLaCamara = (ipEstacion.addr == ipCamara.addr);

        if (!estado->esperandoRespuesta)
        {
            // Todavia no se le pidio ARP: se cuenta como "sigue como estaba"
            // hasta que se verifique en el proximo ciclo (2s despues).
            etharp_request(netifAP, &ipEstacion);
            estado->esperandoRespuesta = true;
            cantidadRespondieron++;
            if (esLaCamara)
                _camaraConectada = _camaraConectada; // sin cambios este ciclo
            continue;
        }

        struct eth_addr *macResuelta;
        const ip4_addr_t *ipResuelta;
        s8_t idx = etharp_find_addr(netifAP, &ipEstacion, &macResuelta, &ipResuelta);
        bool respondio = (idx >= 0);
        estado->esperandoRespuesta = false;

        if (!respondio)
        {
            uint16_t aid = 0;
            if (esp_wifi_ap_get_sta_aid(staWifi.mac, &aid) == ESP_OK)
                esp_wifi_deauth_sta(aid);

            if (esLaCamara)
                _camaraConectada = false;
        }
        else
        {
            cantidadRespondieron++;
            if (esLaCamara)
                _camaraConectada = true;
        }
    }

    _cantidadDispositivos = cantidadRespondieron;
}

int networkMonitor_getCantidadDispositivos() { return _cantidadDispositivos; }
bool networkMonitor_isCamaraConectada() { return _camaraConectada; }