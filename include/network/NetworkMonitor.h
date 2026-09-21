#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <cstdint>

/**
 * @file NetworkMonitor.h
 * @brief Verifica con ARP activo qué dispositivos del AP siguen conectados.
 */


/**
 * @brief Chequea las estaciones asociadas (ARP activo), desasocia las que
 * no responden y actualiza cantidad de dispositivos y estado de la cámara.
 */
void networkMonitor_loop();


/**
 * @brief Cantidad de dispositivos que respondieron el último chequeo ARP.
 */
int networkMonitor_getCantidadDispositivos();


/**
 * @brief true si la cámara (CAM_IP) respondió el último chequeo.
 */
bool networkMonitor_isCamaraConectada();


#endif // NETWORK_MONITOR_H