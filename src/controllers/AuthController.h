#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <ESPAsyncWebServer.h>

/**
 * @file AuthController.h
 * @brief Rutas HTTP de autenticación.
 *
 * Rutas registradas:
 *   POST /api/auth/login      → login con usuario y contraseña
 *   POST /api/auth/invitado   → login como invitado
 *   POST /api/auth/logout     → cierre de sesión
 *   POST /api/auth/heartbeat  → mantener sesión activa
 *   GET  /api/auth/perfil     → datos del usuario logueado
 */

/**
 * @brief Registra las rutas de autenticación en el servidor.
 * @param server Instancia del servidor web.
 */
void registrarAuthController(AsyncWebServer& server);

#endif // AUTH_CONTROLLER_H
