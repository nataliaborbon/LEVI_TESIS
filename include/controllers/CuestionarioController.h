#ifndef CUESTIONARIO_CONTROLLER_H
#define CUESTIONARIO_CONTROLLER_H

#include <ESPAsyncWebServer.h>

/**
 * @file CuestionarioController.h
 * @brief Rutas HTTP de gestión de cuestionarios.
 *
 * Rutas registradas:
 *   GET    /api/cuestionarios              → listar (profesor: los suyos, tutor: todos)
 *   POST   /api/cuestionarios              → crear cuestionario completo
 *   PUT    /api/cuestionarios/:id          → editar título y puntaje
 *   DELETE /api/cuestionarios/:id          → eliminar
 *   PATCH  /api/cuestionarios/:id/iniciar  → pendiente → en_progreso
 *   PATCH  /api/cuestionarios/:id/pausar   → en_progreso → pausado
 *   PATCH  /api/cuestionarios/:id/reanudar → pausado → en_progreso
 *   PATCH  /api/cuestionarios/:id/finalizar→ en_progreso|pausado → finalizado
 *   GET    /api/cuestionarios/:id/revision → detalle con respuestas del alumno
 */

/**
 * @brief Registra las rutas de cuestionarios en el servidor.
 * @param server Instancia del servidor web.
 */
void registrarCuestionarioController(AsyncWebServer& server);

#endif // CUESTIONARIO_CONTROLLER_H
