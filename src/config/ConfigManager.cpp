#include "ConfigManager.h"

// ---------------------------------------------------------------------------
// Helpers privados
// ---------------------------------------------------------------------------

String ConfigManager::_leer(const char* key) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    String valor = prefs.getString(key, "");
    prefs.end();
    return valor;
}

bool ConfigManager::_guardar(const char* key, const String& valor) {
    if (valor.length() == 0) return false;

    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    bool ok = prefs.putString(key, valor) > 0;
    prefs.end();

    if (ok) {
        Serial.printf("[Config] Clave '%s' guardada en NVS.\n", key);
    } else {
        Serial.printf("[Config] Error al guardar clave '%s' en NVS.\n", key);
    }

    return ok;
}

// ---------------------------------------------------------------------------
// Estado de configuración
// ---------------------------------------------------------------------------

bool ConfigManager::clavesConfiguradas() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    bool ok = prefs.isKey(KEY_CLAVE_PROFESOR) && prefs.isKey(KEY_CLAVE_TUTOR);
    prefs.end();
    return ok;
}

bool ConfigManager::claveProfesorConfigurada() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    bool ok = prefs.isKey(KEY_CLAVE_PROFESOR);
    prefs.end();
    return ok;
}

bool ConfigManager::claveTutorConfigurada() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    bool ok = prefs.isKey(KEY_CLAVE_TUTOR);
    prefs.end();
    return ok;
}

// ---------------------------------------------------------------------------
// Guardar claves
// ---------------------------------------------------------------------------

bool ConfigManager::guardarClaveProfesor(const String& clave) {
    return _guardar(KEY_CLAVE_PROFESOR, clave);
}

bool ConfigManager::guardarClaveTutor(const String& clave) {
    return _guardar(KEY_CLAVE_TUTOR, clave);
}

// ---------------------------------------------------------------------------
// Verificar claves
// ---------------------------------------------------------------------------

bool ConfigManager::verificarClaveProfesor(const String& clave) {
    return clave == _leer(KEY_CLAVE_PROFESOR);
}

bool ConfigManager::verificarClaveTutor(const String& clave) {
    return clave == _leer(KEY_CLAVE_TUTOR);
}

bool ConfigManager::verificarClavePorRol(const String& clave, const String& rol) {
    if (rol == "profesor") return verificarClaveProfesor(clave);
    if (rol == "tutor")    return verificarClaveTutor(clave);
    return false;
}
