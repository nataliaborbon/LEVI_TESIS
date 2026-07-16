#include "network/WiFiAP.h"

void initWiFiAP()
{
    Serial.println("[WiFi] Configurando Access Point...");

    WiFi.softAPConfig(CYD_IP, CYD_GATEWAY, CYD_SUBNET);

    if (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD))
    {
        Serial.println("[WiFi] Error: no se pudo crear la red WiFi.");
        return;
    }

    Serial.printf(
        "[WiFi] Red '%s' creada.\n"
        "[WiFi] Acceso por:\n"
        "   http://%s\n"
        "   http://levi.local\n",
        WIFI_SSID,
        WiFi.softAPIP().toString().c_str());

    if (MDNS.begin("levi"))
    {
        Serial.println("[mDNS] Nombre publicado: levi.local");
    }
    else
    {
        Serial.println("[mDNS] Error al iniciar mDNS.");
    }
}