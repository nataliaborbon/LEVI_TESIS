#include "sd_manager.h"
#include "html_content.h" 

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Escribiendo archivo en: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("❌ Fallo al abrir el archivo para escribir");
        return;
    }
    if(file.print(message)){
        Serial.println("✅ Archivo escrito y guardado en la SD");
    } else {
        Serial.println("❌ Fallo en la escritura");
    }
    file.close();
}

bool initSD() {
    if(!SD.begin(SD_CS)){
        Serial.println("❌ Error: No se pudo montar la tarjeta SD.");
        return false;
    }
    Serial.println("✅ Tarjeta SD montada correctamente.");
    return true;
}

void writeHtmlToSD() {
    writeFile(SD, "/index.html", aplicacionHTML);
}