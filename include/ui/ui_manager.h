#ifndef UI_MANAGER_H
#define UI_MANAGER_H

// Inicializa la pantalla, el táctil y LVGL.
// Si isFirstBoot es true, cargará la pantalla de setup.
// Si es false, cargará las pantallas normales.
void ui_init(bool isFirstBoot);

// Esta función debe llamarse dentro del loop() principal de main.cpp
// Mantiene viva la interfaz gráfica.
void ui_loop();

#endif