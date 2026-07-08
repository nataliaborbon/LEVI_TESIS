#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>

/** * @brief Inicializa el servidor asíncrono y define las rutas HTTP.
 * Se encarga de despachar el index.html y el archivo opencv.js
 * directamente desde la memoria SD hacia el cliente conectado.
 */
void initWebServer();

#endif // WEB_SERVER_H