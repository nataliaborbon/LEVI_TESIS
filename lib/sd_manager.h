#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

/// @brief Pin Chip Select para el módulo SD de la placa CYD.
#define SD_CS 5  

/** * @brief Inicializa la tarjeta SD usando el bus SPI de la CYD.
 * @return true si la SD se montó correctamente, false en caso de error.
 */
bool initSD();

/** * @brief Escribe o sobreescribe un archivo de texto en la SD.
 * @param fs Referencia al sistema de archivos (SD).
 * @param path Ruta absoluta del archivo (ej: "/index.html").
 * @param message Contenido a escribir en el archivo.
 */
void writeFile(fs::FS &fs, const char * path, const char * message);

/** * @brief Toma la variable global 'aplicacionHTML' y la guarda físicamente
 * en la SD como 'index.html'.
 */
void writeHtmlToSD();

#endif // SD_MANAGER_H