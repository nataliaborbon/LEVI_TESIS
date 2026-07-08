#ifndef PREGUNTA_OPCION_REPOSITORY_H
#define PREGUNTA_OPCION_REPOSITORY_H

#include <Arduino.h>
#include "../../models/Models.h"
#include "../DatabaseManager.h"

// ===========================================================================
// PREGUNTA REPOSITORY
// ===========================================================================

/**
 * @file PreguntaOpcionRepository.h
 * @brief Acceso a datos de las tablas preguntas y opciones.
 *
 * Se agrupan en un mismo archivo porque opciones siempre se
 * manipulan en conjunto con sus preguntas y nunca de forma aislada.
 */

/**
 * @brief Acceso a datos de la tabla preguntas.
 *
 * Flujo de creación de una pregunta:
 *   1. crear()                    → inserta con idOpcionCorrecta = NULL.
 *   2. OpcionRepository::crear()  → inserta las opciones (2 a 4).
 *   3. asignarOpcionCorrecta()    → completa el campo con el id real.
 */
class PreguntaRepository {
public:
    /**
     * @brief Devuelve la instancia única.
     * @return Referencia al PreguntaRepository.
     */
    static PreguntaRepository& getInstance() {
        static PreguntaRepository instance;
        return instance;
    }

    /**
     * @brief Inserta una pregunta sin idOpcionCorrecta (queda NULL).
     * @param p Pregunta a insertar (idPregunta e idOpcionCorrecta se ignoran).
     * @return DbResult con ok=true e id generado si tuvo éxito.
     */
    DbResult crear(const Pregunta& p);

    /**
     * @brief Asigna la opción correcta tras haber insertado todas las opciones.
     * @param idPregunta       Id de la pregunta a actualizar.
     * @param idOpcionCorrecta Id de la opción correcta (de last_insert_rowid).
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult asignarOpcionCorrecta(int idPregunta, int idOpcionCorrecta);

    /**
     * @brief Registra la opción elegida por el alumno.
     * @param idPregunta      Id de la pregunta respondida.
     * @param idOpcionElegida Id de la opción que eligió el alumno.
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult guardarRespuesta(int idPregunta, int idOpcionElegida);

    /**
     * @brief Busca una pregunta por su id.
     * @param idPregunta Id a buscar.
     * @return Pregunta encontrada, o struct con idPregunta=0 si no existe.
     */
    Pregunta buscarPorId(int idPregunta);

    /**
     * @brief Lista todas las preguntas de un cuestionario ordenadas por id.
     * @param idCuestionario Id del cuestionario.
     * @param buffer         Array destino.
     * @param maxSize        Capacidad máxima del array.
     * @return Cantidad de preguntas encontradas.
     */
    int listarPorCuestionario(int idCuestionario, Pregunta* buffer, int maxSize);

    /**
     * @brief Actualiza el texto y puntajes de una pregunta.
     * @note No actualiza idOpcionCorrecta ni idOpcionElegida.
     * @param p Pregunta con los datos nuevos (idPregunta debe ser válido).
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult actualizar(const Pregunta& p);

    /**
     * @brief Elimina una pregunta por su id.
     * @param idPregunta Id de la pregunta a eliminar.
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult eliminar(int idPregunta);

    /**
     * @brief Limpia todas las respuestas del alumno de un cuestionario.
     * @param idCuestionario Id del cuestionario a limpiar.
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult limpiarRespuestas(int idCuestionario);

    /**
     * @brief Obtiene la primera pregunta sin responder de un cuestionario.
     * @details Usada al reanudar para saber desde dónde continuar.
     * @param idCuestionario Id del cuestionario.
     * @return Pregunta con idOpcionElegida=0, o struct con idPregunta=0 si todas respondidas.
     */
    Pregunta obtenerPrimeraSinResponder(int idCuestionario);

    /**
     * @brief Cuenta el total de preguntas de un cuestionario.
     * @param idCuestionario Id del cuestionario.
     * @return Cantidad total de preguntas.
     */
    int contarTotal(int idCuestionario);

    /**
     * @brief Cuenta las preguntas ya respondidas de un cuestionario.
     * @param idCuestionario Id del cuestionario.
     * @return Cantidad de preguntas con idOpcionElegida != NULL.
     */
    int contarRespondidas(int idCuestionario);

    /**
     * @brief Lista las preguntas de un cuestionario con el detalle de revisión.
     * @details Hace JOIN con opciones para obtener el texto de la opción
     * elegida y la correcta. Usado por profesor y tutor para revisar resultados.
     * @param idCuestionario Id del cuestionario.
     * @param buffer         Array destino.
     * @param maxSize        Capacidad máxima del array.
     * @return Cantidad de preguntas encontradas.
     */
    int listarRevision(int idCuestionario, PreguntaRevision* buffer, int maxSize);

    /**
     * @brief Obtiene la siguiente pregunta sin responder para mostrar al alumno.
     * @details No incluye cuál es la correcta para no revelarla.
     * Las opciones se cargan desde OpcionRepository por separado en el Service.
     * @param idCuestionario Id del cuestionario activo.
     * @param result         PreguntaAlumno a completar (sin opciones aún).
     * @return true si hay una pregunta pendiente, false si el cuestionario terminó.
     */
    bool obtenerSiguienteParaAlumno(int idCuestionario, PreguntaAlumno& result);

private:
    PreguntaRepository() {}
    PreguntaRepository(const PreguntaRepository&)            = delete;
    PreguntaRepository& operator=(const PreguntaRepository&) = delete;

    /**
     * @brief Convierte la fila actual de un stmt en un struct Pregunta.
     * @param stmt Statement con una fila lista para leer.
     * @return Pregunta con los datos de la fila.
     */
    Pregunta _filaAPregunta(sqlite3_stmt* stmt);
};


// ===========================================================================
// OPCION REPOSITORY
// ===========================================================================

/**
 * @brief Acceso a datos de la tabla opciones.
 *
 * Cada pregunta puede tener entre 2 y 4 opciones.
 * El campo esCorrecta no existe en la BD: se determina comparando
 * idOpcion con el idOpcionCorrecta de la pregunta correspondiente.
 */
class OpcionRepository {
public:
    /**
     * @brief Devuelve la instancia única.
     * @return Referencia al OpcionRepository.
     */
    static OpcionRepository& getInstance() {
        static OpcionRepository instance;
        return instance;
    }

    /**
     * @brief Inserta una opción para una pregunta.
     * @param idPregunta  Id de la pregunta a la que pertenece.
     * @param textoOpcion Texto de la opción.
     * @return DbResult con ok=true e id generado si tuvo éxito.
     */
    DbResult crear(int idPregunta, const String& textoOpcion);

    /**
     * @brief Busca una opción por su id.
     * @param idOpcion Id a buscar.
     * @return Opcion encontrada, o struct con idOpcion=0 si no existe.
     */
    Opcion buscarPorId(int idOpcion);

    /**
     * @brief Lista todas las opciones de una pregunta en formato para el alumno.
     * @details No incluye cuál es la correcta.
     * @param idPregunta Id de la pregunta.
     * @param buffer     Array destino.
     * @param maxSize    Capacidad máxima del array.
     * @return Cantidad de opciones encontradas.
     */
    int listarParaAlumno(int idPregunta, OpcionAlumno* buffer, int maxSize);

    /**
     * @brief Lista todas las opciones de una pregunta con todos sus datos.
     * @param idPregunta Id de la pregunta.
     * @param buffer     Array destino.
     * @param maxSize    Capacidad máxima del array.
     * @return Cantidad de opciones encontradas.
     */
    int listarPorPregunta(int idPregunta, Opcion* buffer, int maxSize);

    /**
     * @brief Actualiza el texto de una opción.
     * @param o Opcion con el texto nuevo (idOpcion debe ser válido).
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult actualizar(const Opcion& o);

    /**
     * @brief Elimina una opción por su id.
     * @param idOpcion Id de la opción a eliminar.
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult eliminar(int idOpcion);

    /**
     * @brief Elimina todas las opciones de una pregunta.
     * @param idPregunta Id de la pregunta cuyas opciones se eliminan.
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult eliminarPorPregunta(int idPregunta);

private:
    OpcionRepository() {}
    OpcionRepository(const OpcionRepository&)            = delete;
    OpcionRepository& operator=(const OpcionRepository&) = delete;

    /**
     * @brief Convierte la fila actual de un stmt en un struct Opcion.
     * @param stmt Statement con una fila lista para leer.
     * @return Opcion con los datos de la fila.
     */
    Opcion _filaAOpcion(sqlite3_stmt* stmt);
};

#endif // PREGUNTA_OPCION_REPOSITORY_H
