#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

/**
 * @file SDManager.h
 * @brief Inicialización y operaciones básicas sobre la tarjeta SD.
 *
 * La SD aloja dos recursos:
 *   - opencv.js : librería de visión artificial servida con caché.
 *   - eyetracker.db : base de datos SQLite del sistema.
 */

/// @brief Pin Chip Select para el módulo SD del CYD (ESP32-2432S028R).
#define SD_CS 5

/**
 * @brief Monta la tarjeta SD usando el bus SPI del CYD.
 * @return true si la SD se montó correctamente.
 */
bool initSD();

/**
 * @brief Escribe o sobreescribe un archivo de texto en la SD.
 * @param fs    Sistema de archivos destino.
 * @param path  Ruta absoluta del archivo (ej: "/opencv.js").
 * @param data  Contenido a escribir.
 * @return true si la escritura fue exitosa.
 */
bool writeFile(fs::FS& fs, const char* path, const char* data);

#endif // SD_MANAGER_H
