#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "services/RespuestaService.h" // EstadoExamenResumen

// Inicializa la pantalla, el táctil y LVGL.
// Si isFirstBoot es true, cargará la pantalla de setup.
// Si es false, cargará las pantallas normales.
void ui_init(bool isFirstBoot);

// Esta función debe llamarse dentro del loop() principal de main.cpp
// Mantiene viva la interfaz gráfica y actualiza dispositivos/cámara/examen.
void ui_loop(const EstadoExamenResumen &resumenExamen);

#endif