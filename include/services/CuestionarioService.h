#ifndef CUESTIONARIO_SERVICE_H
#define CUESTIONARIO_SERVICE_H

#include <Arduino.h>
#include "../storage/models/Models.h"

/**
 * @file CuestionarioService.h
 * @brief Lógica de negocio para la gestión de cuestionarios.
 *
 * Responsabilidades:
 *   - Validar datos antes de crear o editar un cuestionario.
 *   - Validar transiciones de estado (pendiente → en_progreso, etc).
 *   - Coordinar PreguntaRepository y OpcionRepository al crear/editar.
 *   - Calcular puntaje al finalizar.
 */

/**
 * @brief Resultado estándar de operaciones de cuestionario.
 */
struct CuestionarioResult {
    bool   ok      = false;
    String mensaje = "";
    int    id      = 0;
};

class CuestionarioService {
public:
    /**
     * @brief Devuelve la instancia única.
     */
    static CuestionarioService& getInstance() {
        static CuestionarioService instance;
        return instance;
    }

    /**
     * @brief Crea un cuestionario completo con todas sus preguntas y opciones.
     *
     * Valida:
     *   - Que el título no esté vacío ni repetido para ese usuario.
     *   - Que haya al menos una pregunta.
     *   - Que cada pregunta tenga entre 2 y 4 opciones.
     *   - Que exactamente una opción por pregunta tenga esCorrecta=true.
     *
     * @param c         Cuestionario a crear (sin idCuestionario).
     * @param preguntas Array de PreguntaCompleta con sus opciones.
     * @param cant      Cantidad de preguntas en el array.
     * @return CuestionarioResult con ok=true e id generado si tuvo éxito.
     */
    CuestionarioResult crear(const Cuestionario& c,
                             const PreguntaCompleta* preguntas, int cant);

    /**
     * @brief Edita el título, puntaje y sobreescribe todas las preguntas
     * @note Solo se puede editar si el estado es 'pendiente'.
     * @param c Cuestionario con los datos nuevos (idCuestionario debe ser válido).
     * @return CuestionarioResult con ok=true si tuvo éxito.
     */
    CuestionarioResult editar(const Cuestionario& c, const PreguntaCompleta* preguntas, int cant);

    /**
     * @brief Trae el cuestionario con todas sus preguntas y opciones para enviarlo al Frontend.
     */
    bool obtenerCompleto(int idCuestionario, Cuestionario& c, PreguntaCompleta* bufferPreguntas, int& cantPreguntas);

    /**
     * @brief Elimina un cuestionario y todas sus preguntas y opciones.
     * @note Solo se puede eliminar si el estado es 'pendiente' o 'finalizado'.
     * @param idCuestionario Id del cuestionario a eliminar.
     * @param idUsuario      Id del profesor dueño (para verificar pertenencia).
     * @return CuestionarioResult con ok=true si tuvo éxito.
     */
    CuestionarioResult eliminar(int idCuestionario, int idUsuario);

    /**
     * @brief Inicia un cuestionario (pendiente → en_progreso).
     * @note Falla si ya hay otro cuestionario en_progreso.
     * @param idCuestionario Id del cuestionario a iniciar.
     * @param idUsuario      Id del profesor dueño.
     * @return CuestionarioResult con ok=true si tuvo éxito.
     */
    CuestionarioResult iniciar(int idCuestionario, int idUsuario);

    /**
     * @brief Pausa un cuestionario (en_progreso → pausado).
     * @param idCuestionario Id del cuestionario a pausar.
     * @param idUsuario      Id del profesor dueño.
     * @return CuestionarioResult con ok=true si tuvo éxito.
     */
    CuestionarioResult pausar(int idCuestionario, int idUsuario);

    /**
     * @brief Reanuda un cuestionario pausado (pausado → en_progreso).
     * @note Falla si ya hay otro cuestionario en_progreso.
     *       Limpia las respuestas anteriores del alumno.
     * @param idCuestionario Id del cuestionario a reanudar.
     * @param idUsuario      Id del profesor dueño.
     * @return CuestionarioResult con ok=true si tuvo éxito.
     */
    CuestionarioResult reanudar(int idCuestionario, int idUsuario);

    /**
     * @brief Finaliza un cuestionario manualmente (en_progreso|pausado → finalizado).
     * @note Calcula el puntaje con las respuestas que haya hasta el momento.
     * @param idCuestionario Id del cuestionario a finalizar.
     * @param idUsuario      Id del profesor dueño.
     * @param tiempoSegundos Tiempo transcurrido desde que inició.
     * @return CuestionarioResult con ok=true si tuvo éxito.
     */
    CuestionarioResult finalizar(int idCuestionario, int idUsuario);

    /**
     * @brief Lista los cuestionarios del profesor en formato resumido.
     * @param idUsuario Id del profesor.
     * @param buffer    Array destino.
     * @param maxSize   Capacidad máxima del array.
     * @return Cantidad de cuestionarios encontrados.
     */
    int listarProfesor(int idUsuario, CuestionarioResumenProfesor* buffer,
                       int maxSize);

    /**
     * @brief Lista todos los cuestionarios en formato resumido para el tutor.
     * @param buffer  Array destino.
     * @param maxSize Capacidad máxima del array.
     * @return Cantidad de cuestionarios encontrados.
     */
    int listarTutor(CuestionarioResumenTutor* buffer, int maxSize);

    /**
     * @brief Obtiene el detalle completo de un cuestionario para revisión.
     * @param idCuestionario Id del cuestionario.
     * @param buffer         Array de PreguntaRevision destino.
     * @param maxSize        Capacidad máxima del array.
     * @return Cantidad de preguntas encontradas, -1 si el cuestionario no existe.
     */
    int obtenerRevision(int idCuestionario, PreguntaRevision* buffer, int maxSize);

    /**
     * @brief Suma tiempo al examen activo basado en los heartbeats del alumno
     * @param idCuestionario Id del cuestionario
     *  */ 
    void procesarHeartbeatCronometro(int idCuestionario);

private:

    int _idCuestionarioTimer = 0;
    int _tiempoAcumuladoSeg = 0;

    void _iniciarCronometro(int idCuestionario);
    void _pausarCronometro(int idCuestionario);
    int _tiempoTranscurridoSeg(int idCuestionario);

    CuestionarioService() {}
    CuestionarioService(const CuestionarioService&)            = delete;
    CuestionarioService& operator=(const CuestionarioService&) = delete;

    /**
     * @brief Calcula el puntaje total sumando los puntajes de las preguntas respondidas.
     * @param idCuestionario Id del cuestionario.
     * @return Puntaje calculado.
     */
    float _calcularPuntaje(int idCuestionario);

    /**
     * @brief Verifica que el cuestionario pertenezca al usuario dado.
     * @param idCuestionario Id del cuestionario.
     * @param idUsuario      Id del profesor.
     * @return true si el cuestionario pertenece al usuario.
     */
    bool _esDuenio(int idCuestionario, int idUsuario);
};

#endif // CUESTIONARIO_SERVICE_H
