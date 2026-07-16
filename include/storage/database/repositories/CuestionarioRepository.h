#ifndef CUESTIONARIO_REPOSITORY_H
#define CUESTIONARIO_REPOSITORY_H

#include <Arduino.h>
#include "../../models/Models.h"
#include "../DatabaseManager.h"

/**
 * @file CuestionarioRepository.h
 * @brief Acceso a datos de la tabla cuestionarios.
 *
 * Responsabilidades:
 *   - CRUD sobre la tabla cuestionarios.
 *   - Cambios de estado del cuestionario.
 *   - Persistencia del resultado final del alumno.
 *   - Listados resumidos con JOINs para los paneles de profesor y tutor.
 */
class CuestionarioRepository {
public:
    /**
     * @brief Devuelve la instancia única.
     * @return Referencia al CuestionarioRepository.
     */
    static CuestionarioRepository& getInstance() {
        static CuestionarioRepository instance;
        return instance;
    }

    /**
     * @brief Inserta un cuestionario nuevo con estado 'pendiente'.
     * @param c Cuestionario a insertar (idCuestionario se ignora).
     * @return DbResult con ok=true e id generado si tuvo éxito.
     */
    DbResult crear(const Cuestionario& c);

    /**
     * @brief Busca un cuestionario por su id.
     * @param idCuestionario Id a buscar.
     * @return Cuestionario encontrado, o struct con idCuestionario=0 si no existe.
     */
    Cuestionario buscarPorId(int idCuestionario);

    /**
     * @brief Devuelve el cuestionario en estado 'en_progreso', si existe.
     * @return Cuestionario activo, o struct con idCuestionario=0 si no hay ninguno.
     */
    Cuestionario obtenerActivo();

    /**
     * @brief Actualiza titulo y puntajeParaAprobar de un cuestionario.
     * @note No actualiza el estado.
     * @param c Cuestionario con los datos nuevos (idCuestionario debe ser válido).
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult actualizar(const Cuestionario& c);

    /**
     * @brief Cambia el estado de un cuestionario.
     * @param idCuestionario Id del cuestionario.
     * @param nuevoEstado    "pendiente" | "en_progreso" | "pausado" | "finalizado"
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult cambiarEstado(int idCuestionario, const String& nuevoEstado);

    /**
     * @brief Guarda el resultado final y marca el cuestionario como 'finalizado'.
     * @param idCuestionario    Id del cuestionario.
     * @param puntajeObtenido   Puntaje calculado por RespuestaService.
     * @param fechaFinalizacion Fecha y hora de finalización (ISO 8601).
     * @param tiempoSegundos    Tiempo total que tardó el alumno.
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult guardarResultado(int idCuestionario, float puntajeObtenido,
                              const String& fechaFinalizacion, int tiempoSegundos);

    /**
     * @brief Elimina un cuestionario por su id.
     * @param idCuestionario Id del cuestionario a eliminar.
     * @return DbResult con ok=true si tuvo éxito.
     */
    DbResult eliminar(int idCuestionario);

    /**
     * @brief Verifica si ya existe un cuestionario con ese título para ese usuario.
     * @param idUsuario Id del profesor.
     * @param titulo    Título a verificar.
     * @return true si ya existe.
     */
    bool existeTitulo(int idUsuario, const String& titulo);

    /**
     * @brief Verifica si ya existe un cuestionario con ese título para ese usuario
     * excluyendo un id específico (para validar al editar).
     * @param idUsuario      Id del profesor.
     * @param titulo         Título a verificar.
     * @param excluirId      Id del cuestionario a excluir.
     * @return true si ya existe otro cuestionario con ese título.
     */
    bool existeTituloExcluyendo(int idUsuario, const String& titulo, int excluirId);

    /**
     * @brief Verifica si hay algún cuestionario en estado 'en_progreso'.
     * @return true si hay uno activo.
     */
    bool hayUnoEnProgreso();

    // -----------------------------------------------------------------------
    // Listados con JOIN para los paneles
    // -----------------------------------------------------------------------

    /**
     * @brief Lista los cuestionarios de un profesor en formato resumido.
     * @param idUsuario Id del profesor.
     * @param buffer    Array destino.
     * @param maxSize   Capacidad máxima del array.
     * @return Cantidad de cuestionarios encontrados.
     */
    int listarResumenProfesor(int idUsuario, CuestionarioResumenProfesor* buffer,
                              int maxSize);

    /**
     * @brief Lista todos los cuestionarios en formato resumido para el tutor.
     * @details Incluye la materia del profesor via JOIN con usuarios.
     * @param buffer  Array destino.
     * @param maxSize Capacidad máxima del array.
     * @return Cantidad de cuestionarios encontrados.
     */
    int listarResumenTutor(CuestionarioResumenTutor* buffer, int maxSize);

private:
    CuestionarioRepository() {}
    CuestionarioRepository(const CuestionarioRepository&)            = delete;
    CuestionarioRepository& operator=(const CuestionarioRepository&) = delete;

    /**
     * @brief Convierte la fila actual de un stmt en un struct Cuestionario.
     * @param stmt Statement con una fila lista para leer.
     * @return Cuestionario con los datos de la fila.
     */
    Cuestionario _filaACuestionario(sqlite3_stmt* stmt);
};

#endif // CUESTIONARIO_REPOSITORY_H
