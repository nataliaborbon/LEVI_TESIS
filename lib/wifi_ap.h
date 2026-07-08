#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <WiFi.h>
#include "data.h"

/** * @brief Configura y levanta el ESP32 CYD como Access Point (Router).
 * Utiliza las credenciales e IPs definidas en data.h.
 */
void initWiFiAP();

#endif // WIFI_AP_H