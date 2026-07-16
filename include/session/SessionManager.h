#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <Arduino.h>
#include "../storage/models/Models.h"

/**
 * @file SessionManager.h
 * @brief Gestión de sesiones activas en RAM.
 *
 * Maneja dos slots independientes:
 *   - Sesión panel: profesor, tutor o invitado (uno a la vez, con prioridad).
 *   - Sesión alumno: un solo dispositivo a la vez.
 *
 * Las sesiones no persisten entre reinicios de la ESP32.
 * Los tokens se generan con esp_random() al iniciar cada sesión.
 *
 * Timeouts:
 *   - Profesor/Tutor : 10 minutos sin heartbeat.
 *   - Invitado       :  5 minutos sin heartbeat.
 *   - Alumno         : 30 segundos sin heartbeat (libera slot, no borra respuestas).
 */

// ---------------------------------------------------------------------------
// Structs internos de sesión
// ---------------------------------------------------------------------------

/**
 * @brief Sesión activa del panel (profesor, tutor o invitado).
 */
struct SesionPanel {
    String        token            = "";
    String        rol              = "";  ///< "profesor" | "tutor" | "invitado"
    int           idUsuario        = 0;   ///< 0 si es invitado
    String        nombre           = "";  ///< Para mostrar en el panel
    unsigned long ultimaActividad  = 0;   ///< millis() del último heartbeat
    bool          activa           = false;
};

/**
 * @brief Sesión activa del alumno.
 */
struct SesionAlumno {
    String        token           = "";
    unsigned long ultimaActividad = 0;
    bool          activa          = false;
};

/**
 * @brief Pregunta del modo invitado almacenada en RAM.
 * @note Nunca se persiste en la BD. Se borra al cerrar la sesión del invitado.
 */
struct PreguntaInvitado {
    String textoOpregunta  = "";
    String opciones[4];
    int    cantOpciones    = 0;
    bool   cargada         = false;
};

// ---------------------------------------------------------------------------
// Resultado de operaciones de sesión
// ---------------------------------------------------------------------------

/**
 * @brief Resultado de un intento de inicio de sesión.
 */
struct SesionResult {
    bool   ok       = false;
    String token    = "";
    String mensaje  = "";
};

// ---------------------------------------------------------------------------
// SessionManager
// ---------------------------------------------------------------------------

class SessionManager {
public:
    /**
     * @brief Devuelve la instancia única.
     * @return Referencia al SessionManager.
     */
    static SessionManager& getInstance() {
        static SessionManager instance;
        return instance;
    }

    // -----------------------------------------------------------------------
    // Sesión panel
    // -----------------------------------------------------------------------

    /**
     * @brief Intenta iniciar sesión en el panel.
     *
     * Aplica la lógica de prioridad:
     *   tutor (3) > profesor (2) > invitado (1)
     *
     * Si el nuevo rol tiene mayor prioridad que el actual, expulsa la sesión
     * existente y crea una nueva. Si tiene igual o menor prioridad, rechaza.
     *
     * @param rol       "profesor" | "tutor" | "invitado"
     * @param idUsuario Id del usuario. 0 si es invitado.
     * @param nombre    Nombre para mostrar en el panel.
     * @return SesionResult con ok=true y token si se creó la sesión.
     */
    SesionResult iniciarSesionPanel(const String& rol, int idUsuario,
                                    const String& nombre);

    /**
     * @brief Cierra la sesión del panel activa.
     * @note Si era invitado, también limpia la PreguntaInvitado en RAM.
     */
    void cerrarSesionPanel();

    /**
     * @brief Verifica si un token de panel es válido.
     * @param token Token a verificar.
     * @return true si el token corresponde a la sesión activa.
     */
    bool verificarTokenPanel(const String& token);

    /**
     * @brief Actualiza el timestamp de actividad de la sesión panel.
     * @param token Token de la sesión a actualizar.
     * @return true si el token era válido y se actualizó.
     */
    bool heartbeatPanel(const String& token);

    /**
     * @brief Devuelve la sesión panel activa (solo lectura).
     * @return Referencia constante a la SesionPanel.
     */
    const SesionPanel& getSesionPanel() const { return _panel; }

    /**
     * @brief Indica si hay una sesión panel activa.
     * @return true si hay una sesión activa.
     */
    bool hayPanelActivo() const { return _panel.activa; }

    /**
     * @brief Actualiza en tiempo real el nombre y apellido del usuario
     */

    void actualizarNombrePanel(const String& nuevoNombre);

    // -----------------------------------------------------------------------
    // Sesión alumno
    // -----------------------------------------------------------------------

    /**
     * @brief Intenta iniciar sesión del alumno.
     * @note Solo puede haber un alumno conectado a la vez.
     * @return SesionResult con ok=true y token si se creó la sesión.
     *         ok=false si ya hay un alumno conectado.
     */
    SesionResult iniciarSesionAlumno();

    /**
     * @brief Cierra la sesión del alumno.
     * @note No borra las respuestas en BD, solo libera el slot.
     */
    void cerrarSesionAlumno();

    /**
     * @brief Verifica si un token de alumno es válido.
     * @param token Token a verificar.
     * @return true si el token corresponde a la sesión activa del alumno.
     */
    bool verificarTokenAlumno(const String& token);

    /**
     * @brief Actualiza el timestamp de actividad de la sesión alumno.
     * @param token Token de la sesión a actualizar.
     * @return true si el token era válido y se actualizó.
     */
    bool heartbeatAlumno(const String& token);

    /**
     * @brief Indica si hay un alumno conectado.
     * @return true si hay una sesión de alumno activa.
     */
    bool hayAlumnoActivo() const { return _alumno.activa; }

    // -----------------------------------------------------------------------
    // Pregunta del invitado (RAM)
    // -----------------------------------------------------------------------

    /**
     * @brief Guarda la pregunta del invitado en RAM.
     * @param pregunta Struct con texto y opciones de la pregunta.
     */
    void guardarPreguntaInvitado(const PreguntaInvitado& pregunta);

    /**
     * @brief Devuelve la pregunta del invitado almacenada en RAM.
     * @return Referencia constante a la PreguntaInvitado.
     */
    const PreguntaInvitado& getPreguntaInvitado() const { return _preguntaInvitado; }

    /**
     * @brief Limpia la pregunta del invitado de la RAM.
     */
    void limpiarPreguntaInvitado();

    // -----------------------------------------------------------------------
    // Tick de timeouts (llamar desde loop())
    // -----------------------------------------------------------------------

    /**
     * @brief Verifica y aplica timeouts de sesiones inactivas.
     *
     * Debe llamarse periódicamente desde el loop() de main.cpp.
     * Cierra automáticamente las sesiones que superaron su tiempo límite.
     */
    void tick();

private:
    SessionManager() {}
    SessionManager(const SessionManager&)            = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    SesionPanel      _panel;
    SesionAlumno     _alumno;
    PreguntaInvitado _preguntaInvitado;

    /// @brief Timeout en ms para profesor y tutor (10 minutos).
    static constexpr unsigned long TIMEOUT_PANEL_MS    = 10UL * 60UL * 1000UL;

    /// @brief Timeout en ms para invitado (5 minutos).
    static constexpr unsigned long TIMEOUT_INVITADO_MS =  5UL * 60UL * 1000UL;

    /// @brief Timeout en ms para alumno (30 segundos).
    static constexpr unsigned long TIMEOUT_ALUMNO_MS   = 30UL * 1000UL;

    /**
     * @brief Genera un token aleatorio de 32 caracteres hexadecimales.
     * @return Token generado con esp_random().
     */
    String _generarToken();

    /**
     * @brief Devuelve el nivel de prioridad de un rol.
     * @param rol "tutor" | "profesor" | "invitado"
     * @return 3 para tutor, 2 para profesor, 1 para invitado, 0 para desconocido.
     */
    int _prioridad(const String& rol);
};

#endif // SESSION_MANAGER_H
