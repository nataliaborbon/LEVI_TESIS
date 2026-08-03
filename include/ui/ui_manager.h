#ifndef UI_MANAGER_H
#define UI_MANAGER_H

/**
 * @brief Inicializa la interfaz gráfica y la pantalla táctil.
 * @param isFirstBoot Indica si es el primer arranque del sistema.
 */
void ui_init(bool isFirstBoot);

/**
 * @brief Bucle principal de la interfaz gráfica.
 * Este bucle se encarga de actualizar la interfaz, manejar la entrada táctil y controlar la suspensión por inactividad.
 */
void ui_loop();

#endif