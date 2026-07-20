#include "storage/database/repositories/CuestionarioRepository.h"
#include <esp_heap_caps.h>

Cuestionario CuestionarioRepository::_filaACuestionario(sqlite3_stmt* stmt) {
    Cuestionario c;
    c.idCuestionario     = sqlite3_column_int(stmt, 0);
    c.idUsuario          = sqlite3_column_int(stmt, 1);
    c.titulo             = String((const char*)sqlite3_column_text(stmt, 2));
    c.puntajeParaAprobar = (float)sqlite3_column_double(stmt, 3);
    c.estado             = String((const char*)sqlite3_column_text(stmt, 4));

    c.puntajeObtenido   = sqlite3_column_type(stmt, 5) != SQLITE_NULL
                          ? (float)sqlite3_column_double(stmt, 5) : 0.0f;

    const char* fecha   = (const char*)sqlite3_column_text(stmt, 6);
    c.fechaFinalizacion = fecha ? String(fecha) : "";

    c.tiempoSegundos    = sqlite3_column_type(stmt, 7) != SQLITE_NULL
                          ? sqlite3_column_int(stmt, 7) : 0;
    return c;
}

DbResult CuestionarioRepository::crear(const Cuestionario& c)
{
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        INSERT INTO cuestionarios (idUsuario, titulo, puntajeParaAprobar, estado)
        VALUES (?, ?, ?, 'pendiente');
    )";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    rc = sqlite3_bind_int(stmt, 1, c.idUsuario);

    rc = sqlite3_bind_text(stmt, 2, c.titulo.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_bind_double(stmt, 3, c.puntajeParaAprobar);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_DONE)
    {
        result.ok = true;
        result.id = (int)sqlite3_last_insert_rowid(db);
    }
    else
    {
        result.mensaje = String("step error: ") + sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);

    return result;
}

Cuestionario CuestionarioRepository::buscarPorId(int idCuestionario)
{

    Cuestionario c;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        SELECT idCuestionario, idUsuario, titulo, puntajeParaAprobar,
               estado, puntajeObtenido, fechaFinalizacion, tiempoSegundos
        FROM cuestionarios
        WHERE idCuestionario = ?;
    )";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return c;
    }

    rc = sqlite3_bind_int(stmt, 1, idCuestionario);

    rc = sqlite3_step(stmt);


    if (rc == SQLITE_ROW)
    {
        c = _filaACuestionario(stmt);
    }

    sqlite3_finalize(stmt);

    return c;
}

Cuestionario CuestionarioRepository::obtenerActivo()
{

    Cuestionario c;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        SELECT idCuestionario, idUsuario, titulo, puntajeParaAprobar,
               estado, puntajeObtenido, fechaFinalizacion, tiempoSegundos
        FROM cuestionarios
        WHERE estado='en_progreso'
        LIMIT 1;
    )";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return c;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        c = _filaACuestionario(stmt);
    }

    sqlite3_finalize(stmt);

    return c;
}

DbResult CuestionarioRepository::actualizar(const Cuestionario& c)
{

    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        UPDATE cuestionarios
        SET titulo=?, puntajeParaAprobar=?
        WHERE idCuestionario=?;
    )";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    rc = sqlite3_bind_text(stmt, 1, c.titulo.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_bind_double(stmt, 2, c.puntajeParaAprobar);

    rc = sqlite3_bind_int(stmt, 3, c.idCuestionario);


    rc = sqlite3_step(stmt);

    result.ok = (rc == SQLITE_DONE);

    if (!result.ok)
    {
        result.mensaje = String("step error: ") + sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);
    return result;
}

DbResult CuestionarioRepository::cambiarEstado(int idCuestionario,
                                               const String& nuevoEstado)
{

    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "UPDATE cuestionarios SET estado=? WHERE idCuestionario=?;",
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_text(stmt, 1, nuevoEstado.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, idCuestionario);

    rc = sqlite3_step(stmt);

    result.ok = (rc == SQLITE_DONE);

    if (!result.ok)
    {
        result.mensaje = String("step error: ") + sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);

    return result;
}

DbResult CuestionarioRepository::guardarResultado(int idCuestionario,
                                                  float puntajeObtenido,
                                                  const String& fechaFinalizacion,
                                                  int tiempoSegundos)
{

    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        UPDATE cuestionarios
        SET puntajeObtenido=?,
            fechaFinalizacion=?,
            tiempoSegundos=?,
            estado='finalizado'
        WHERE idCuestionario=?;
    )";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_double(stmt, 1, puntajeObtenido);
    sqlite3_bind_text(stmt, 2, fechaFinalizacion.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, tiempoSegundos);
    sqlite3_bind_int(stmt, 4, idCuestionario);

    rc = sqlite3_step(stmt);

    result.ok = (rc == SQLITE_DONE);

    if (!result.ok)
    {
        result.mensaje = String("step error: ") + sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);

    return result;
}

DbResult CuestionarioRepository::eliminar(int idCuestionario)
{

    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "DELETE FROM cuestionarios WHERE idCuestionario=?;",
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_int(stmt, 1, idCuestionario);

    rc = sqlite3_step(stmt);

    result.ok = (rc == SQLITE_DONE);

    if (!result.ok)
    {
        result.mensaje = String("step error: ") + sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);

    return result;
}

bool CuestionarioRepository::existeTitulo(int idUsuario, const String& titulo)
{

    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "SELECT 1 FROM cuestionarios WHERE idUsuario = ? AND titulo = ? LIMIT 1;",
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, idUsuario);
    sqlite3_bind_text(stmt, 2, titulo.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    bool existe = (rc == SQLITE_ROW);

    sqlite3_finalize(stmt);

    return existe;
}

bool CuestionarioRepository::existeTituloExcluyendo(int idUsuario,
                                                    const String& titulo,
                                                    int excluirId)
{

    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "SELECT 1 FROM cuestionarios WHERE idUsuario=? AND titulo=? AND idCuestionario!=? LIMIT 1;",
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, idUsuario);
    sqlite3_bind_text(stmt, 2, titulo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, excluirId);

    rc = sqlite3_step(stmt);

    bool existe = (rc == SQLITE_ROW);

    sqlite3_finalize(stmt);

    return existe;
}

bool CuestionarioRepository::hayUnoEnProgreso()
{

    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        "SELECT 1 FROM cuestionarios WHERE estado='en_progreso' LIMIT 1;",
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    rc = sqlite3_step(stmt);

    bool hay = (rc == SQLITE_ROW);

    sqlite3_finalize(stmt);

    return hay;
}

int CuestionarioRepository::listarResumenProfesor(int idUsuario,
                                                  CuestionarioResumenProfesor* buffer,
                                                  int maxSize)
{

    sqlite3* db = DatabaseManager::getInstance().getDB();
    int count = 0;

    const char* sql = R"(
        SELECT c.idCuestionario, c.titulo, c.estado,
               c.puntajeObtenido, c.puntajeParaAprobar,
               COUNT(p.idPregunta) as cantPreguntas
        FROM cuestionarios c
        LEFT JOIN preguntas p ON p.idCuestionario = c.idCuestionario
        WHERE c.idUsuario = ?
        GROUP BY c.idCuestionario
        ORDER BY c.idCuestionario DESC;
    )";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, idUsuario);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && count < maxSize)
    {
        CuestionarioResumenProfesor& r = buffer[count++];

        r.idCuestionario     = sqlite3_column_int(stmt, 0);
        r.titulo             = String((const char*)sqlite3_column_text(stmt, 1));
        r.estado             = String((const char*)sqlite3_column_text(stmt, 2));

        r.puntajeObtenido =
            sqlite3_column_type(stmt, 3) != SQLITE_NULL
                ? (float)sqlite3_column_double(stmt, 3)
                : 0.0f;

        r.puntajeParaAprobar = (float)sqlite3_column_double(stmt, 4);
        r.cantPreguntas      = sqlite3_column_int(stmt, 5);

        r.aprobado =
            (r.puntajeObtenido >= r.puntajeParaAprobar) &&
            r.estado == "finalizado";
    }

    sqlite3_finalize(stmt);

    return count;
}

int CuestionarioRepository::listarResumenTutor(CuestionarioResumenTutor* buffer,
                                               int maxSize)
{

    sqlite3* db = DatabaseManager::getInstance().getDB();
    int count = 0;

    const char* sql = R"(
        SELECT c.idCuestionario, c.titulo, c.estado,
               c.puntajeObtenido, c.puntajeParaAprobar,
               COUNT(p.idPregunta) as cantPreguntas,
               u.materia
        FROM cuestionarios c
        JOIN usuarios u ON c.idUsuario = u.idUsuario
        LEFT JOIN preguntas p ON p.idCuestionario = c.idCuestionario
        GROUP BY c.idCuestionario
        ORDER BY c.idCuestionario DESC;
    )";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return 0;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && count < maxSize)
    {
        CuestionarioResumenTutor& r = buffer[count++];

        r.idCuestionario     = sqlite3_column_int(stmt, 0);
        r.titulo             = String((const char*)sqlite3_column_text(stmt, 1));
        r.estado             = String((const char*)sqlite3_column_text(stmt, 2));

        r.puntajeObtenido =
            sqlite3_column_type(stmt, 3) != SQLITE_NULL
                ? (float)sqlite3_column_double(stmt, 3)
                : 0.0f;

        r.puntajeParaAprobar = (float)sqlite3_column_double(stmt, 4);
        r.cantPreguntas      = sqlite3_column_int(stmt, 5);

        r.aprobado =
            (r.puntajeObtenido >= r.puntajeParaAprobar) &&
            r.estado == "finalizado";

        const char* materia =
            (const char*)sqlite3_column_text(stmt, 6);

        r.materia = materia ? String(materia) : "";
    }
    sqlite3_finalize(stmt);

    return count;
}