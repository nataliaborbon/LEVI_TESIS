#ifndef CAMERA_MONITOR_H
#define CAMERA_MONITOR_H

#include <Arduino.h>

/**
 * @file CameraMonitor.h
 * @brief Detección de presencia de un dispositivo en CAM_IP dentro de la red del AP.
 *
 * El CYD opera como Access Point y mantiene su propia tabla ARP con los
 * dispositivos que tuvieron tráfico reciente en la red local. Este módulo
 * consulta esa tabla para verificar si existe una entrada resuelta para
 * la IP fija de la cámara (CAM_IP, en NetworkConfig.h), sin importar si
 * esa IP fue asignada por el DHCP del CYD o configurada manualmente del
 * lado del dispositivo. Cualquier equipo que use esa IP y esté enviando
 * tráfico (como el stream MJPEG constante de la cámara) va a aparecer
 * resuelto en la tabla.
 *
 * No genera tráfico de red propio: es una consulta local a la pila lwIP.
 */

void cameraMonitor_loop();
bool cameraMonitor_isConectada();

#endif // CAMERA_MONITOR_H