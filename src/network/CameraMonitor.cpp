#include "network/CameraMonitor.h"
#include "config/NetworkConfig.h"

#include "lwip/netif.h"
#include "lwip/etharp.h"

static const uint32_t INTERVALO_MS = 2000;

enum class EstadoChequeo { PURGAR_Y_PEDIR, VERIFICAR };

static uint32_t ultimoCambio = 0;
static EstadoChequeo estadoActual = EstadoChequeo::PURGAR_Y_PEDIR;
static bool camaraConectada = false;

static struct netif *buscarNetifDelAP()
{
    struct netif *iter;
    NETIF_FOREACH(iter)
    {
        if (iter->ip_addr.u_addr.ip4.addr == static_cast<uint32_t>(CYD_IP))
        {
            return iter;
        }
    }
    return nullptr;
}

void cameraMonitor_loop()
{
    uint32_t ahora = millis();
    if (ahora - ultimoCambio < INTERVALO_MS) return;
    ultimoCambio = ahora;

    struct netif *netifAP = buscarNetifDelAP();
    if (netifAP == nullptr)
    {
        camaraConectada = false;
        return;
    }

    ip4_addr_t ipBuscada;
    ipBuscada.addr = static_cast<uint32_t>(CAM_IP);

    if (estadoActual == EstadoChequeo::PURGAR_Y_PEDIR)
    {
        etharp_cleanup_netif(netifAP);
        etharp_request(netifAP, &ipBuscada);
        estadoActual = EstadoChequeo::VERIFICAR;
    }
    else
    {
        struct eth_addr *macEncontrada;
        const ip4_addr_t *ipResuelta;
        s8_t indice = etharp_find_addr(netifAP, &ipBuscada, &macEncontrada, &ipResuelta);

        camaraConectada = (indice >= 0);
        estadoActual = EstadoChequeo::PURGAR_Y_PEDIR;
    }
}

bool cameraMonitor_isConectada()
{
    return camaraConectada;
}