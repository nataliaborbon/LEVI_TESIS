#ifndef RESPUESTA_SERVICE_H
#define RESPUESTA_SERVICE_H

#include <Arduino.h>
#include "../storage/models/Models.h"
#include "../session/SessionManager.h"

/**
 * @file RespuestaService.h
 * @brief Lógica del flujo del alumno durante el examen.
 *
 * Responsabilidades:
 *   - Iniciar y cerrar la sesión del alumno.
 *   - Determinar el estado actual del examen para mostrar en pantalla.
 *   - Registrar la respuesta del alumno y evaluar si fue correcta.
 *   - Manejar la pregunta del invitado en RAM.
 */

/**
 * @brief Estado actual del examen para el alumno.
 */
struct EstadoAlumno {
    String estado          = "esperando"; // "esperando"|"en_progreso"|"pausado"|"finalizado"|"invitado"
    bool   hayPregunta     = false;

    // Datos de la pregunta actual (si hay)
    PreguntaAlumno pregunta;

    // Datos del resultado (si finalizó)
    float  puntajeObtenido    = 0.0f;
    float  puntajeParaAprobar = 0.0f;
    bool   aprobado           = false;
    int    tiempoSegundos     = 0;
};

/**
 * @brief Resultado de registrar una respuesta del alumno.
 */
struct RespuestaResult {
    bool   ok          = false;
    bool   fueCorrecto = false;
    bool   finalizo    = false; // true si era la última pregunta
    String mensaje     = "";
};

class RespuestaService {
public:
    /**
     * @brief Devuelve la instancia única.
     */
    static RespuestaService& getInstance() {
        static RespuestaService instance;
        return instance;
    }

    /**
     * @brief Inicia la sesión del alumno.
     * @return SesionResult con token si tuvo éxito.
     */
    SesionResult iniciarSesion();

    /**
     * @brief Devuelve el estado actual del examen para mostrar en la pantalla del alumno.
     *
     * Determina qué mostrar según el cuestionario activo:
     *   - "esperando"   → no hay cuestionario en_progreso ni invitado activo
     *   - "en_progreso" → hay cuestionario activo, devuelve la siguiente pregunta sin responder
     *   - "pausado"     → el cuestionario está pausado
     *   - "finalizado"  → el cuestionario finalizó, devuelve resultado
     *   - "invitado"    → hay una pregunta de invitado en RAM
     *
     * @return EstadoAlumno con toda la información necesaria para el frontend.
     */
    EstadoAlumno obtenerEstado();

    /**
     * @brief Registra la respuesta del alumno a la pregunta actual.
     *
     * Si es modo normal: guarda idOpcionElegida en BD y evalúa si fue correcta.
     * Si es modo invitado: solo devuelve el texto de la opción elegida (sin BD).
     *
     * @param idPregunta Id de la pregunta respondida (0 si es invitado).
     * @param idOpcion   Id de la opción elegida (índice 0-3 si es invitado).
     * @return RespuestaResult con si fue correcta y si finalizó el examen.
     */
    RespuestaResult responder(int idPregunta, int idOpcion);

private:
    RespuestaService() {}
    RespuestaService(const RespuestaService&)            = delete;
    RespuestaService& operator=(const RespuestaService&) = delete;
};

#endif // RESPUESTA_SERVICE_H
