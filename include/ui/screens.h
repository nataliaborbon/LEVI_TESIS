#ifndef SCREENS_H
#define SCREENS_H

#include <lvgl.h>

// Inicializa el Tileview con las 3 pantallas normales
void ui_screen_main_init();

// Inicializa la pantalla de primer uso
void ui_screen_setup_init();

// Función para actualizar el Usuario
void ui_update_usuario(const char * nombre);

// Funcion para detectar dispositivos
void ui_update_dispositivos(int cantidad);

// Función para actualizar la Cámara
void ui_update_camara(bool lista);

// Función para actualizar el examen
void ui_update_examen(const char * estado, const char * titulo, int numeroPregunta, int totalPreguntas);

#endif