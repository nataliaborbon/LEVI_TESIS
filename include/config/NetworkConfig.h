#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <IPAddress.h>

/**
 * @file NetworkConfig.h
 * @brief Credenciales WiFi e IPs fijas de la red local del sistema.
 *
 * El CYD opera como Access Point. Las IPs son estáticas para que
 * la ESP32-CAM siempre sea encontrable en la misma dirección.
 */

// ---------------------------------------------------------------------------
// Credenciales de la red WiFi
// ---------------------------------------------------------------------------

#ifndef WIFI_SSID
/// @brief Nombre de la red WiFi creada por el CYD.
#define WIFI_SSID "LEVI"
#endif

#ifndef WIFI_PASSWORD
/// @brief Contraseña de la red WiFi creada por el CYD.
#define WIFI_PASSWORD "12345678"
#endif

// ---------------------------------------------------------------------------
// IPs estáticas de la red
// ---------------------------------------------------------------------------

/// @brief IP del CYD (Access Point y servidor web).
const IPAddress CYD_IP(192, 168, 4, 1);

/// @brief Gateway de la red (mismo que el AP).
const IPAddress CYD_GATEWAY(192, 168, 4, 1);

/// @brief Máscara de subred.
const IPAddress CYD_SUBNET(255, 255, 255, 0);

/// @brief IP fija de la ESP32-CAM en la red.
const IPAddress CAM_IP(192, 168, 4, 50);

#endif // NETWORK_CONFIG_H
