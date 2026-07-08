#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "../session/SessionManager.h"

/**
 * @file Middleware.h
 * @brief Funciones auxiliares para verificación de tokens y respuestas HTTP.
 *
 * Todos los controllers usan estas funciones para:
 *   - Extraer el token del header Authorization.
 *   - Verificar que la sesión sea válida antes de procesar el request.
 *   - Enviar respuestas JSON estandarizadas.
 */

/**
 * @brief Extrae el token del header "Authorization: Bearer <token>".
 * @param request Request HTTP entrante.
 * @return Token extraído, o cadena vacía si no existe o tiene formato incorrecto.
 */
String extraerToken(AsyncWebServerRequest* request);

/**
 * @brief Verifica que haya una sesión panel válida para el request.
 * @param request  Request HTTP entrante.
 * @param rolRequerido Rol mínimo requerido. Vacío = cualquier rol de panel.
 * @return true si la sesión es válida y tiene el rol correcto.
 */
bool verificarSesionPanel(AsyncWebServerRequest* request,
                           const String& rolRequerido = "");

/**
 * @brief Verifica que haya una sesión de alumno válida para el request.
 * @param request Request HTTP entrante.
 * @return true si la sesión de alumno es válida.
 */
bool verificarSesionAlumno(AsyncWebServerRequest* request);

/**
 * @brief Envía una respuesta JSON con código de estado.
 * @param request Request HTTP entrante.
 * @param codigo  Código HTTP (200, 201, 400, 401, 403, 404, 409, 500).
 * @param json    Cuerpo JSON como String.
 */
void enviarJSON(AsyncWebServerRequest* request, int codigo, const String& json);

/**
 * @brief Envía una respuesta de error estandarizada.
 * @param request  Request HTTP entrante.
 * @param codigo   Código HTTP de error.
 * @param mensaje  Mensaje de error legible.
 */
void enviarError(AsyncWebServerRequest* request, int codigo, const String& mensaje);

/**
 * @brief Envía una respuesta 200 OK con mensaje de éxito.
 * @param request  Request HTTP entrante.
 * @param mensaje  Mensaje de éxito.
 */
void enviarOk(AsyncWebServerRequest* request, const String& mensaje);

#endif // MIDDLEWARE_H
