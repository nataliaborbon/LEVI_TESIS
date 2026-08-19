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

    // Válido solo si la operación fue finalizar()/finalizarComoAlumno() y ok=true.
    ResultadoFinalizacion resultado;
};

class CuestionarioService {
public:
    static CuestionarioService& getInstance() {
        static CuestionarioService instance;
        return instance;
    }

    CuestionarioResult crear(const Cuestionario& c,
                             const PreguntaCompleta* preguntas, int cant);

    CuestionarioResult editar(const Cuestionario& c, const PreguntaCompleta* preguntas, int cant);

    bool obtenerCompleto(int idCuestionario, Cuestionario& c, PreguntaCompleta* bufferPreguntas, int& cantPreguntas);

    CuestionarioResult eliminar(int idCuestionario, int idUsuario);

    CuestionarioResult iniciar(int idCuestionario, int idUsuario);

    CuestionarioResult pausar(int idCuestionario, int idUsuario);

    CuestionarioResult reanudar(int idCuestionario, int idUsuario);

    CuestionarioResult finalizar(int idCuestionario, int idUsuario);

    /**
     * @brief Finaliza el cuestionario cuando el alumno responde la última pregunta
     * (en_progreso → finalizado).
     * @note A diferencia de finalizar(), no requiere verificación de dueño (el
     *       alumno no es dueño del cuestionario), pero sí exige que estén
     *       todas las preguntas respondidas.
     * @param idCuestionario Id del cuestionario a finalizar.
     * @return CuestionarioResult con ok=true y el resultado calculado si tuvo éxito.
     */
    CuestionarioResult finalizarComoAlumno(int idCuestionario);

    int listarProfesor(int idUsuario, CuestionarioResumenProfesor* buffer,
                       int maxSize);

    int listarTutor(CuestionarioResumenTutor* buffer, int maxSize);

    int obtenerRevision(int idCuestionario, PreguntaRevision* buffer, int maxSize);

    void procesarHeartbeatCronometro(int idCuestionario);

private:

    int _idCuestionarioTimer = 0;
    int _tiempoAcumuladoSeg = 0;

    void _iniciarCronometro(int idCuestionario);
    void _pausarCronometro(int idCuestionario);
    void _reanudarCronometro(int idCuestionario);
    int _tiempoTranscurridoSeg(int idCuestionario);

    CuestionarioService() {}
    CuestionarioService(const CuestionarioService&)            = delete;
    CuestionarioService& operator=(const CuestionarioService&) = delete;

    float _calcularPuntaje(int idCuestionario);

    /**
     * @brief Suma el puntajeCorrecta de TODAS las preguntas del cuestionario,
     * sin importar si están respondidas (es el máximo teórico posible).
     * @param idCuestionario Id del cuestionario.
     * @return Puntaje máximo posible.
     */
    float _calcularPuntajeMaximo(int idCuestionario);

    /**
     * @brief Lógica común de finalización, usada por finalizar() y
     * finalizarComoAlumno(). Calcula puntaje, puntaje máximo y tiempo,
     * persiste el resultado en BD, y llena result.resultado.
     * @note Asume que ya se validó estado y permisos antes de llamarla.
     * @param idCuestionario Id del cuestionario a finalizar.
     * @param result         Struct a llenar (ok/mensaje/resultado).
     */
    void _finalizarInterno(int idCuestionario, CuestionarioResult& result);

    bool _esDuenio(int idCuestionario, int idUsuario);
};

#endif // CUESTIONARIO_SERVICE_H