#ifndef USUARIO_CONTROLLER_H
#define USUARIO_CONTROLLER_H

#include <ESPAsyncWebServer.h>

/**
 * @file UsuarioController.h
 * @brief Rutas HTTP de gestión de usuarios.
 *
 * Rutas registradas:
 *   POST   /api/usuarios              → crear usuario (requiere clave maestra)
 *   DELETE /api/usuarios/:usuario     → eliminar usuario (requiere clave maestra)
 *   PUT    /api/usuarios/perfil       → editar perfil propio
 *   PUT    /api/usuarios/password     → cambiar contraseña propia
 *   GET    /api/usuarios/profesores   → listar profesores (tutor y profesor)
 *   GET    /api/usuarios/tutores      → listar tutores (tutor y profesor)
 */

/**
 * @brief Registra las rutas de usuarios en el servidor.
 * @param server Instancia del servidor web.
 */
void registrarUsuarioController(AsyncWebServer& server);

#endif // USUARIO_CONTROLLER_H
