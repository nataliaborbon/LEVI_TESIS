#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <cstdint>

/**
 * @file NetworkMonitor.h
 * @brief Cantidad de dispositivos en el AP (lista WiFi) y estado de la cámara (ping activo).
 */


/**
 * @brief Arranca la sesión de ping hacia la cámara. Llamar una vez en
 * setup(), después de que el AP ya esté levantado.
 */
void networkMonitor_init();


/**
 * @brief Actualiza la cantidad de dispositivos conectados.
 */
void networkMonitor_loop();


/**
 * @brief Cantidad de dispositivos conectados al AP (incluye a la cámara si está conectada).
 */
int networkMonitor_getCantidadDispositivos();


/**
 * @brief true si la cámara (CAM_IP) respondió el último ping.
 */
bool networkMonitor_isCamaraConectada();


#endif // NETWORK_MONITOR_H