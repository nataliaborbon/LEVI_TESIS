#ifndef RESPUESTA_CONTROLLER_H
#define RESPUESTA_CONTROLLER_H

#include <ESPAsyncWebServer.h>

/**
 * @file RespuestaController.h
 * @brief Rutas HTTP del flujo del alumno y del invitado.
 *
 * Rutas registradas:
 *   POST /api/alumno/iniciar    → obtener token de sesión alumno
 *   GET  /api/alumno/estado     → qué mostrar en pantalla
 *   POST /api/alumno/responder  → registrar opción elegida
 *   POST /api/alumno/heartbeat  → mantener sesión activa
 *   POST /api/invitado/pregunta → guardar pregunta en RAM
 */

/**
 * @brief Registra las rutas del alumno e invitado en el servidor.
 * @param server Instancia del servidor web.
 */
void registrarRespuestaController(AsyncWebServer& server);

#endif // RESPUESTA_CONTROLLER_H
