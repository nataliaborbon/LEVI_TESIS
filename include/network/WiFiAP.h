#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <WiFi.h>
#include <ESPmDNS.h>

#include "../config/NetworkConfig.h"

/**
 * @file WiFiAP.h
 * @brief Configura el CYD como Access Point WiFi.
 *
 * Además de crear la red LEVI, publica el nombre
 * levi.local mediante mDNS para permitir el acceso
 * sin necesidad de conocer la dirección IP.
 *
 * El sistema puede accederse mediante:
 *   http://levi.local
 *   http://192.168.4.1
 */

/**
 * @brief Levanta el Access Point con IP estática.
 *
 * Configura la IP, gateway y subred antes de crear
 * la red WiFi, garantizando que el CYD siempre sea
 * accesible en CYD_IP.
 *
 * También inicia el servicio mDNS para publicar
 * el nombre levi.local.
 */
void initWiFiAP();

#endif // WIFI_AP_H