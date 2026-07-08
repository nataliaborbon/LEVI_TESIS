#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>

/**
 * @file WebServer.h
 * @brief Inicialización del servidor web asíncrono.
 *
 * Gestiona dos orígenes de archivos estáticos:
 *   - LittleFS: build de React (index.html + assets).
 *   - SD:       opencv.js, servido con caché de un año.
 *
 * Además, redirige automáticamente los accesos realizados
 * mediante la dirección IP hacia levi.local para mantener
 * una URL consistente y amigable para el usuario.
 *
 * Las rutas de la API (/api/...) son registradas por
 * cada Controller al momento de su inicialización.
 */

extern AsyncWebServer server;

/**
 * @brief Registra las rutas estáticas e inicia el servidor.
 *
 * Debe llamarse después de montar LittleFS y SD,
 * ya que sirve archivos desde ambos sistemas.
 */
void initWebServer();

#endif // WEB_SERVER_H