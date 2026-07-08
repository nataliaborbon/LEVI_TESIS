#include "SDManager.h"

bool initSD() {
    if (!SD.begin(SD_CS)) {
        Serial.println("[SD] Error: no se pudo montar la tarjeta SD.");
        return false;
    }
    Serial.println("[SD] Tarjeta SD montada correctamente.");
    return true;
}

bool writeFile(fs::FS& fs, const char* path, const char* data) {
    Serial.printf("[SD] Escribiendo: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.printf("[SD] Error: no se pudo abrir %s para escritura.\n", path);
        return false;
    }

    bool ok = file.print(data);
    file.close();

    if (ok) {
        Serial.printf("[SD] Archivo %s guardado correctamente.\n", path);
    } else {
        Serial.printf("[SD] Error: fallo al escribir %s.\n", path);
    }

    return ok;
}
