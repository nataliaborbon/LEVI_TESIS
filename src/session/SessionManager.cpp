#include "SessionManager.h"
#include "../ui/screens.h"
#include "../storage/database/repositories/CuestionarioRepository.h"
#include "../services/CuestionarioService.h"

// ---------------------------------------------------------------------------
// Helpers privados
// ---------------------------------------------------------------------------

String SessionManager::_generarToken() {
    String token = "";
    for (int i = 0; i < 16; i++) {
        byte b = (byte)(esp_random() % 256);
        if (b < 16) token += "0";
        token += String(b, HEX);
    }
    return token;
}

int SessionManager::_prioridad(const String& rol) {
    if (rol == "tutor")    return 3;
    if (rol == "profesor") return 2;
    if (rol == "invitado") return 1;
    return 0;
}

// ---------------------------------------------------------------------------
// Sesión panel
// ---------------------------------------------------------------------------

SesionResult SessionManager::iniciarSesionPanel(const String& rol, int idUsuario,
                                                 const String& nombre) {
    SesionResult result;
    int prioridadNuevo   = _prioridad(rol);
    int prioridadActual  = _panel.activa ? _prioridad(_panel.rol) : 0;

    if (_panel.activa) {
        if (prioridadNuevo > prioridadActual) {
            // Expulsar sesión actual para dar paso al de mayor prioridad
            Serial.printf("[Session] Expulsando sesión de '%s' por '%s'.\n",
                          _panel.rol.c_str(), rol.c_str());
            cerrarSesionPanel();
        } else if (prioridadNuevo == prioridadActual) {
            result.mensaje = "Ya hay un " + rol + " conectado.";
            return result;
        } else {
            result.mensaje = "Hay un " + _panel.rol + " conectado con mayor prioridad.";
            return result;
        }
    }

    _panel.token           = _generarToken();
    _panel.rol             = rol;
    _panel.idUsuario       = idUsuario;
    _panel.nombre          = nombre;
    _panel.ultimaActividad = millis();
    _panel.activa          = true;

    Serial.printf("[Session] Sesión panel iniciada: rol=%s nombre=%s\n",
                  rol.c_str(), nombre.c_str());

    if (rol == "profesor" || rol == "tutor") {
        ui_update_usuario(_panel.nombre.c_str());
    }

    result.ok    = true;
    result.token = _panel.token;
    return result;
}

void SessionManager::cerrarSesionPanel() {
    if (!_panel.activa) return;

    Serial.printf("[Session] Sesión panel cerrada: rol=%s\n", _panel.rol.c_str());

    ui_update_usuario("");

    if (_panel.rol == "invitado") limpiarPreguntaInvitado();

    _panel = SesionPanel();
}

bool SessionManager::verificarTokenPanel(const String& token) {
    return _panel.activa && _panel.token == token;
}

bool SessionManager::heartbeatPanel(const String& token) {
    if (!verificarTokenPanel(token)) return false;
    _panel.ultimaActividad = millis();
    return true;
}

void SessionManager::actualizarNombrePanel(const String& nuevoNombre) {
    if (_panel.activa) {
        _panel.nombre = nuevoNombre;
        Serial.printf("[Session] Nombre actualizado en sesión: %s\n", nuevoNombre.c_str());
        
        if (_panel.rol == "profesor" || _panel.rol == "tutor") {
            ui_update_usuario(_panel.nombre.c_str());
        }
    }
}

// ---------------------------------------------------------------------------
// Sesión alumno
// ---------------------------------------------------------------------------

SesionResult SessionManager::iniciarSesionAlumno() {
    SesionResult result;

    if (_alumno.activa) {
        result.mensaje = "Ya hay un alumno conectado.";
        return result;
    }

    _alumno.token           = _generarToken();
    _alumno.ultimaActividad = millis();
    _alumno.activa          = true;

    Serial.println("[Session] Sesión alumno iniciada.");

    result.ok    = true;
    result.token = _alumno.token;
    return result;
}

void SessionManager::cerrarSesionAlumno() {
    if (!_alumno.activa) return;
    Serial.println("[Session] Sesión alumno cerrada.");
    _alumno = SesionAlumno();
}

bool SessionManager::verificarTokenAlumno(const String& token) {
    return _alumno.activa && _alumno.token == token;
}

bool SessionManager::heartbeatAlumno(const String& token) {
    if (!verificarTokenAlumno(token)) return false;
    _alumno.ultimaActividad = millis();

    //Aprovechamos el heartbeat para sumar tiempo
    Cuestionario activo = CuestionarioRepository::getInstance().obtenerActivo();
    if (activo.idCuestionario != 0 && activo.estado == "en_progreso") {
        CuestionarioService::getInstance().procesarHeartbeatCronometro(activo.idCuestionario);
    }

    return true;
}

// ---------------------------------------------------------------------------
// Pregunta del invitado
// ---------------------------------------------------------------------------

void SessionManager::guardarPreguntaInvitado(const PreguntaInvitado& pregunta) {
    _preguntaInvitado = pregunta;
    Serial.println("[Session] Pregunta de invitado guardada en RAM.");
}

void SessionManager::limpiarPreguntaInvitado() {
    _preguntaInvitado = PreguntaInvitado();
    Serial.println("[Session] Pregunta de invitado eliminada de RAM.");
}

// ---------------------------------------------------------------------------
// Tick de timeouts
// ---------------------------------------------------------------------------

void SessionManager::tick() {
    unsigned long ahora = millis();

    // Chequeo sesión panel
    if (_panel.activa) {
        unsigned long timeout = (_panel.rol == "invitado")
                                ? TIMEOUT_INVITADO_MS
                                : TIMEOUT_PANEL_MS;

        if (ahora - _panel.ultimaActividad > timeout) {
            Serial.printf("[Session] Timeout de sesión panel: rol=%s\n",
                          _panel.rol.c_str());
            cerrarSesionPanel();
        }
    }

    // Chequeo sesión alumno
    if (_alumno.activa) {
        if (ahora - _alumno.ultimaActividad > TIMEOUT_ALUMNO_MS) {
            Serial.println("[Session] Timeout de sesión alumno.");
            //Auto-pausar si se cae la conexión ---
            Cuestionario activo = CuestionarioRepository::getInstance().obtenerActivo();
            if (activo.idCuestionario != 0 && activo.estado == "en_progreso") {
                Serial.println("[Session] Examen pausado automáticamente por pérdida de conexión.");
                CuestionarioService::getInstance().pausar(activo.idCuestionario, activo.idUsuario);
            }
            cerrarSesionAlumno();
        }
    }
}
