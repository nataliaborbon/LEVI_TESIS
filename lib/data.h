#ifndef DATA_H
#define DATA_H

#include <Arduino.h>

#ifndef WIFI_SSID
/// @brief Nombre de la red WiFi
#define WIFI_SSID "LEVI"
#endif

#ifndef WIFI_PASSWORD
/// @brief Contraseña de la red WiFi
#define WIFI_PASSWORD "12345678"
#endif

// IPs fijas para la red offline
const IPAddress CYD_IP(192, 168, 4, 1);
const IPAddress CYD_GATEWAY(192, 168, 4, 1);
const IPAddress CYD_SUBNET(255, 255, 255, 0);
const IPAddress S3_IP(192, 168, 4, 50);

#endif // DATA_H