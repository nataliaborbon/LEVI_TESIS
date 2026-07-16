#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>

/**
 * @file Models.h
 * @brief Structs que representan las tablas de la BD y DTOs para consultas con JOIN.
 */

// ===========================================================================
// ENTIDADES BASE (representan tablas de la BD)
// ===========================================================================

/**
 * @brief Representa un registro de la tabla usuarios.
 * @note El alumno no tiene registro en esta tabla.
 */
struct Usuario
{
    int idUsuario = 0;
    String usuario = ""; 
    String nombre = "";
    String apellido = "";
    String rol = "";   
    String materia = ""; 
    String contacto = "";
    String hashPassword = "";
    String salt = "";
};

/**
 * @brief Representa un registro de la tabla cuestionarios.
 *
 * puntajeObtenido, fechaFinalizacion y tiempoSegundos permanecen
 * en su valor por defecto hasta que el alumno finaliza el examen.
 */
struct Cuestionario
{
    int idCuestionario = 0;
    int idUsuario = 0;
    String titulo = "";
    float puntajeParaAprobar = 0.0f;
    String estado = "pendiente"; 
    float puntajeObtenido = 0.0f;
    String fechaFinalizacion = "";
    int tiempoSegundos = 0;
};

/**
 * @brief Representa un registro de la tabla opciones.
 *
 * @note esCorrecta no existe en la BD. Solo se usa al crear una pregunta
 * para identificar cuál opción marcar como correcta en asignarOpcionCorrecta().
 */
struct Opcion
{
    int idOpcion = 0;
    int idPregunta = 0;
    String opcion = "";
    bool esCorrecta = false;
};

/**
 * @brief Representa un registro de la tabla preguntas.
 *
 * idOpcionCorrecta: 0 hasta que se asigna tras insertar las opciones.
 * idOpcionElegida:  0 hasta que el alumno responde.
 */
struct Pregunta
{
    int idPregunta = 0;
    int idCuestionario = 0;
    int idOpcionCorrecta = 0;
    int idOpcionElegida = 0;
    String pregunta = "";
    float puntajeCorrecta = 0.0f;
    float puntajeIncorrecta = 0.0f;
};

// ===========================================================================
// DTOs DE CREACIÓN
// Se usan para recibir datos del frontend al crear/editar.
// ===========================================================================

/**
 * @brief DTO que agrupa una pregunta con sus opciones para creación.
 * @note cantOpciones indica cuántas opciones están cargadas (2 a 4).
 */
struct PreguntaCompleta
{
    Pregunta pregunta;
    Opcion opciones[4];
    int cantOpciones = 0;
};

// ===========================================================================
// DTOs DE LECTURA PARA EL PANEL DEL PROFESOR
// ===========================================================================

/**
 * @brief Resumen de un cuestionario para el panel del profesor.
 *
 * El profesor solo ve sus propios cuestionarios y ya conoce su materia,
 * por eso no se incluye información del profesor ni materia.
 */
struct CuestionarioResumenProfesor
{
    int idCuestionario = 0;
    String titulo = "";
    String estado = "";
    float puntajeObtenido = 0.0f;
    float puntajeParaAprobar = 0.0f;
    bool aprobado = false; 
    int cantPreguntas = 0;
};

/**
 * @brief Resumen de un tutor para el panel del profesor.
 * @note El profesor puede ver la lista de tutores y su contacto.
 */
struct TutorResumen
{
    int idUsuario = 0;
    String nombre = "";
    String apellido = "";
    String contacto = "";
};

// ===========================================================================
// DTOs DE LECTURA PARA EL PANEL DEL TUTOR
// ===========================================================================

/**
 * @brief Resumen de un cuestionario para el panel del tutor.
 *
 * El tutor ve todos los cuestionarios de todos los profesores.
 * Se incluye la materia para identificar a qué asignatura pertenece.
 */
struct CuestionarioResumenTutor
{
    int idCuestionario = 0;
    String titulo = "";
    String estado = "";
    float puntajeObtenido = 0.0f;
    float puntajeParaAprobar = 0.0f;
    bool aprobado = false; 
    int cantPreguntas = 0;
    String materia = ""; 
};

/**
 * @brief Resumen de un profesor para el panel del tutor.
 * @note El tutor puede ver la lista de profesores con su materia y contacto.
 */
struct ProfesorResumen
{
    int idUsuario = 0;
    String nombre = "";
    String apellido = "";
    String materia = "";
    String contacto = "";
};

// ===========================================================================
// DTOs DE REVISIÓN DE CUESTIONARIO FINALIZADO
// Usados por profesor y tutor para ver el detalle del resultado.
// ===========================================================================

/**
 * @brief Detalle de una pregunta respondida para la vista de revisión.
 *
 * Muestra el texto de la opción elegida y la correcta para que
 * el profesor o tutor pueda ver qué respondió el alumno.
 */
struct PreguntaRevision
{
    int idPregunta = 0;
    String textoPregunta = "";
    float puntajeCorrecta = 0.0f;
    float puntajeIncorrecta = 0.0f;
    String opcionElegida = "";  
    String opcionCorrecta = ""; 
    bool fueCorrecto = false;   
};

// ===========================================================================
// DTOs PARA EL PANEL DEL ALUMNO
// ===========================================================================

/**
 * @brief Opción simplificada para mostrar al alumno.
 * @note No incluye si es correcta para no revelar la respuesta.
 */
struct OpcionAlumno
{
    int idOpcion = 0;
    String opcion = "";
};

/**
 * @brief Pregunta activa para mostrar al alumno durante el examen.
 *
 * numeroPregunta y totalPreguntas se calculan en el Service
 * contando las preguntas respondidas y totales del cuestionario.
 */
struct PreguntaAlumno
{
    int idPregunta = 0;
    String textoPregunta = "";
    int numeroPregunta = 0; 
    int totalPreguntas = 0; 
    OpcionAlumno opciones[4];
    int cantOpciones = 0;
};

// ===========================================================================
// RESULTADO DE OPERACIONES DE BD
// Todas las funciones de repository devuelven este struct.
// ===========================================================================

/**
 * @brief Resultado estándar de una operación sobre la base de datos.
 *
 * @param ok      true si la operación fue exitosa.
 * @param mensaje Descripción del error en caso de fallo.
 * @param id      ID generado por un INSERT (lastInsertRowId). 0 si no aplica.
 */
struct DbResult
{
    bool ok = false;
    String mensaje = "";
    int id = 0;
};

#endif // MODELS_H
