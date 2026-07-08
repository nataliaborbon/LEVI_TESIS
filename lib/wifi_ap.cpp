#include "wifi_ap.h"

void initWiFiAP()
{
    Serial.println("Configurando Access Point (CYD)...");

    // Forzamos la IP estática del AP
    WiFi.softAPConfig(CYD_IP, CYD_GATEWAY, CYD_SUBNET);

    if (WiFi.softAP(WIFI_SSID, WIFI_PASSWORD))
    {
        Serial.print("✅ Red creada. Conectate a '");
        Serial.print(WIFI_SSID);
        Serial.print("' y abrí: http://");
        Serial.println(WiFi.softAPIP());
    }
    else
    {
        Serial.println("❌ ERROR: No se pudo crear la red Wi-Fi.");
    }
}