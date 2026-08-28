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
    String estado          = "esperando"; // "esperando"|"en_progreso"|"invitado"
    bool   hayPregunta     = false;
    String tituloCuestionario = "";

    // Datos de la pregunta actual (si hay)
    PreguntaAlumno pregunta;
};

/**
 * @brief Resultado de registrar una respuesta del alumno.
 */
struct RespuestaResult {
    bool   ok          = false;
    bool   fueCorrecto = false;
    bool   finalizo    = false; // true si era la última pregunta
    bool   aprobado    = false; // válido solo si finalizo == true
    String mensaje     = "";

    // Datos de puntaje/tiempo, válidos solo si finalizo == true.
    ResultadoFinalizacion resultado;
};

/**
 * @brief Snapshot liviano del último estado calculado por obtenerEstado().
 *
 * Se actualiza como efecto secundario cada vez que alguien llama a
 * obtenerEstado() (hoy: el handler HTTP de /api/alumno/estado, que ya
 * corre en la tarea del webserver). NO dispara ninguna consulta nueva a
 * la base — es solo una copia en RAM para que loop() en main.cpp pueda
 * leerla sin tocar la SD/SQLite desde una segunda tarea.
 *
 * Usa char[] en vez de String a propósito: son campos que se leen desde
 * otra tarea sin mutex, y un char[] fijo es más simple de copiar sin
 * riesgo que un String (que internamente maneja un puntero a heap).
 */
struct EstadoExamenResumen {
    char  estado[16]     = "esperando";
    int   numeroPregunta = 0;
    int   totalPreguntas = 0;
    char  tituloCuestionario[64] = "";

    // Válidos solo cuando estado == "finalizado" (ver _registrarFinalizacion).
    float puntajeObtenido    = 0;
    float puntajeParaAprobar = 0;
    float puntajeMaximo      = 0;
    float tiempoSegundos     = 0;
    bool  aprobado           = false;
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
     * La pantalla del alumno solo necesita distinguir dos casos reales:
     *   - "en_progreso" → hay cuestionario activo (o invitado), devuelve la pregunta
     *   - "esperando"   → no hay nada para mostrar
     *
     * El resultado de finalización NO viaja en este EstadoAlumno: llega en
     * la respuesta directa de responder() (ver RespuestaResult), porque es
     * en ese momento cuando efectivamente se calcula y persiste. Como
     * efecto secundario, este método también actualiza el resumen cacheado
     * que usa el CYD (ver obtenerResumenCacheado) — salvo mientras haya una
     * finalización reciente "congelada" ahí (ver _registrarFinalizacion).
     *
     * @return EstadoAlumno con toda la información necesaria para el frontend.
     */
    EstadoAlumno obtenerEstado();

    /**
     * @brief Devuelve el último estado calculado, SIN tocar la base.
     *
     * Es una simple lectura de RAM (se actualiza como efecto secundario
     * dentro de obtenerEstado()). Pensado para que main.cpp lo use en su
     * tick de 1s para alimentar la pantalla del CYD, sin generar una
     * segunda consulta a la SD/SQLite desde otra tarea.
     *
     * Justo después de que el alumno termina la última pregunta, este
     * resumen queda "congelado" en estado "finalizado" (con puntaje,
     * tiempo y si aprobó) durante unos segundos, para que el CYD tenga
     * tiempo de mostrar el resultado antes de que vuelva a "esperando".
     *
     * @return Copia del último EstadoExamenResumen conocido.
     */
    EstadoExamenResumen obtenerResumenCacheado() const { return _resumenCache; }

    /**
     * @brief Registra la respuesta del alumno a la pregunta actual.
     *
     * Si es modo normal: guarda idOpcionElegida en BD y evalúa si fue correcta.
     * Si es la última pregunta del cuestionario, delega en
     * CuestionarioService::finalizarComoAlumno() para calcular y persistir
     * el resultado, y lo devuelve ya calculado en RespuestaResult.
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

    /// @brief Lógica real de obtenerEstado() (sin tocar el cache).
    EstadoAlumno _calcularEstado();

    /// @brief Actualiza _resumenCache a partir de un EstadoAlumno recién calculado.
    void _actualizarResumenCache(const EstadoAlumno& estado);

    /**
     * @brief Congela el resumen cacheado en "finalizado" con los datos del
     * resultado, durante _finalizadoHastaMs. Se llama desde responder()
     * cuando la última pregunta se acaba de contestar.
     *
     * Es necesario porque, para cuando el próximo obtenerEstado() se
     * calcule, el cuestionario ya no está "en_progreso" (obtenerActivo()
     * ya no lo encuentra) y _calcularEstado() daría "esperando" — pisando
     * el resultado antes de que la pantalla del CYD llegue a mostrarlo.
     */
    void _registrarFinalizacion(const ResultadoFinalizacion& resultado, bool aprobado, const String& titulo);

    EstadoExamenResumen _resumenCache;

    /// millis() hasta el cual _resumenCache no debe ser pisado por el
    /// cálculo normal de obtenerEstado() (ver _registrarFinalizacion).
    unsigned long _finalizadoHastaMs = 0;
};

#endif // RESPUESTA_SERVICE_H