#include "storage/database/repositories/CuestionarioRepository.h"
#include "esp_heap_caps.h"
#include "config/Limites.h"

static void _logHeapRepo(const char* etiqueta) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    Serial.printf("[%s] Heap libre=%u Max alloc block=%u\n",
                  etiqueta, info.total_free_bytes, info.largest_free_block);
}

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

DbResult CuestionarioRepository::crear(const Cuestionario& c) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        INSERT INTO cuestionarios (idUsuario, titulo, puntajeParaAprobar, estado)
        VALUES (?, ?, ?, 'pendiente');
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_int   (stmt, 1, c.idUsuario);
    sqlite3_bind_text  (stmt, 2, c.titulo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, c.puntajeParaAprobar);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result.ok = true;
        result.id = (int)sqlite3_last_insert_rowid(db);
    } else {
        result.mensaje = String("step error: ") + sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);
    return result;
}

Cuestionario CuestionarioRepository::buscarPorId(int idCuestionario) {
    Cuestionario c;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        SELECT idCuestionario, idUsuario, titulo, puntajeParaAprobar,
               estado, puntajeObtenido, fechaFinalizacion, tiempoSegundos
        FROM cuestionarios WHERE idCuestionario = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return c;

    sqlite3_bind_int(stmt, 1, idCuestionario);

    if (sqlite3_step(stmt) == SQLITE_ROW) c = _filaACuestionario(stmt);

    sqlite3_finalize(stmt);
    return c;
}

Cuestionario CuestionarioRepository::obtenerActivo() {
    Cuestionario c;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        SELECT idCuestionario, idUsuario, titulo, puntajeParaAprobar,
               estado, puntajeObtenido, fechaFinalizacion, tiempoSegundos
        FROM cuestionarios WHERE estado = 'en_progreso' LIMIT 1;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return c;

    if (sqlite3_step(stmt) == SQLITE_ROW) c = _filaACuestionario(stmt);

    sqlite3_finalize(stmt);
    return c;
}

DbResult CuestionarioRepository::actualizar(const Cuestionario& c) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        UPDATE cuestionarios SET titulo = ?, puntajeParaAprobar = ?
        WHERE idCuestionario = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_text  (stmt, 1, c.titulo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, c.puntajeParaAprobar);
    sqlite3_bind_int   (stmt, 3, c.idCuestionario);

    result.ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!result.ok) result.mensaje = String("step error: ") + sqlite3_errmsg(db);

    sqlite3_finalize(stmt);
    return result;
}

DbResult CuestionarioRepository::cambiarEstado(int idCuestionario,
                                                const String& nuevoEstado) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    _logHeapRepo("CAMBIAR-ESTADO pre");

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db,
        "UPDATE cuestionarios SET estado = ? WHERE idCuestionario = ?;",
        -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_text(stmt, 1, nuevoEstado.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 2, idCuestionario);

    result.ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!result.ok) result.mensaje = String("step error: ") + sqlite3_errmsg(db);

    sqlite3_finalize(stmt);

    _logHeapRepo("CAMBIAR-ESTADO post");

    return result;
}

DbResult CuestionarioRepository::guardarResultado(int idCuestionario,
                                                   float puntajeObtenido,
                                                   const String& fechaFinalizacion,
                                                   int tiempoSegundos) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        UPDATE cuestionarios
        SET puntajeObtenido = ?, fechaFinalizacion = ?, tiempoSegundos = ?,
            estado = 'finalizado'
        WHERE idCuestionario = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_double(stmt, 1, puntajeObtenido);
    sqlite3_bind_text  (stmt, 2, fechaFinalizacion.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt, 3, tiempoSegundos);
    sqlite3_bind_int   (stmt, 4, idCuestionario);

    result.ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!result.ok) result.mensaje = String("step error: ") + sqlite3_errmsg(db);

    sqlite3_finalize(stmt);
    return result;
}

DbResult CuestionarioRepository::eliminar(int idCuestionario) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db,
        "DELETE FROM cuestionarios WHERE idCuestionario = ?;",
        -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_int(stmt, 1, idCuestionario);

    result.ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!result.ok) result.mensaje = String("step error: ") + sqlite3_errmsg(db);

    sqlite3_finalize(stmt);
    return result;
}

bool CuestionarioRepository::existeTitulo(int idUsuario, const String& titulo) {
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT 1 FROM cuestionarios WHERE idUsuario = ? AND titulo = ? LIMIT 1;",
        -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int (stmt, 1, idUsuario);
    sqlite3_bind_text(stmt, 2, titulo.c_str(), -1, SQLITE_TRANSIENT);

    bool existe = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return existe;
}

bool CuestionarioRepository::existeTituloExcluyendo(int idUsuario,
                                                     const String& titulo,
                                                     int excluirId) {
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT 1 FROM cuestionarios WHERE idUsuario = ? AND titulo = ? AND idCuestionario != ? LIMIT 1;",
        -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int (stmt, 1, idUsuario);
    sqlite3_bind_text(stmt, 2, titulo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 3, excluirId);

    bool existe = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return existe;
}

bool CuestionarioRepository::hayUnoEnProgreso() {
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT 1 FROM cuestionarios WHERE estado = 'en_progreso' LIMIT 1;",
        -1, &stmt, nullptr) != SQLITE_OK) return false;

    bool hay = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return hay;
}

int CuestionarioRepository::listarResumenProfesor(int idUsuario,
                                                   CuestionarioResumenProfesor* buffer,
                                                   int maxSize) {
    sqlite3* db = DatabaseManager::getInstance().getDB();
    int count = 0;

    const char* sql = R"(
        SELECT idCuestionario, titulo, estado, puntajeObtenido, puntajeParaAprobar
        FROM cuestionarios
        WHERE idUsuario = ?
        ORDER BY idCuestionario DESC;
    )";

    _logHeapRepo("LISTAR-PROF pre");

    sqlite3_stmt* stmt;
    int rcPrep = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rcPrep != SQLITE_OK) {
        Serial.printf("[LISTAR-PROF] prepare rc=%d errmsg=%s\n", rcPrep, sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_int(stmt, 1, idUsuario);

    int rcStep;
    while ((rcStep = sqlite3_step(stmt)) == SQLITE_ROW && count < maxSize) {
        CuestionarioResumenProfesor& r = buffer[count++];
        r.idCuestionario     = sqlite3_column_int(stmt, 0);
        r.titulo             = String((const char*)sqlite3_column_text(stmt, 1));
        r.estado             = String((const char*)sqlite3_column_text(stmt, 2));
        r.puntajeObtenido    = sqlite3_column_type(stmt, 3) != SQLITE_NULL
                               ? (float)sqlite3_column_double(stmt, 3) : 0.0f;
        r.puntajeParaAprobar = (float)sqlite3_column_double(stmt, 4);
        r.aprobado           = (r.puntajeObtenido >= r.puntajeParaAprobar)
                               && r.estado == "finalizado";
    }

    if (rcStep != SQLITE_DONE && rcStep != SQLITE_ROW) {
        Serial.printf("[LISTAR-PROF] step rc=%d errmsg=%s\n", rcStep, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    const char* sqlCount = "SELECT COUNT(*) FROM preguntas WHERE idCuestionario = ?;";
    for (int i = 0; i < count; i++) {
        sqlite3_stmt* stmtCount;
        if (sqlite3_prepare_v2(db, sqlCount, -1, &stmtCount, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtCount, 1, buffer[i].idCuestionario);
            buffer[i].cantPreguntas = (sqlite3_step(stmtCount) == SQLITE_ROW)
                                       ? sqlite3_column_int(stmtCount, 0) : 0;
            sqlite3_finalize(stmtCount);
        } else {
            buffer[i].cantPreguntas = 0;
        }
    }

    _logHeapRepo("LISTAR-PROF post");
    return count;
}

int CuestionarioRepository::listarResumenTutor(CuestionarioResumenTutor* buffer,
                                                int maxSize) {
    sqlite3* db = DatabaseManager::getInstance().getDB();
    int count = 0;
    static int _idUsuarioTemp[MAX_CUESTIONARIOS_LISTADO];

    const char* sql = R"(
        SELECT idCuestionario, idUsuario, titulo, estado,
               puntajeObtenido, puntajeParaAprobar
        FROM cuestionarios
        ORDER BY idCuestionario DESC;
    )";

    _logHeapRepo("LISTAR-TUTOR pre");

    sqlite3_stmt* stmt;
    int rcPrep = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rcPrep != SQLITE_OK) {
        Serial.printf("[LISTAR-TUTOR] prepare rc=%d errmsg=%s\n", rcPrep, sqlite3_errmsg(db));
        return 0;
    }

    int rcStep;
    while ((rcStep = sqlite3_step(stmt)) == SQLITE_ROW && count < maxSize) {
        CuestionarioResumenTutor& r = buffer[count++];
        r.idCuestionario     = sqlite3_column_int(stmt, 0);
        int idUsuario        = sqlite3_column_int(stmt, 1);
        r.titulo             = String((const char*)sqlite3_column_text(stmt, 2));
        r.estado             = String((const char*)sqlite3_column_text(stmt, 3));
        r.puntajeObtenido    = sqlite3_column_type(stmt, 4) != SQLITE_NULL
                               ? (float)sqlite3_column_double(stmt, 4) : 0.0f;
        r.puntajeParaAprobar = (float)sqlite3_column_double(stmt, 5);
        r.aprobado           = (r.puntajeObtenido >= r.puntajeParaAprobar)
                               && r.estado == "finalizado";

        _idUsuarioTemp[count - 1] = idUsuario;
    }

    if (rcStep != SQLITE_DONE && rcStep != SQLITE_ROW) {
        Serial.printf("[LISTAR-TUTOR] step rc=%d errmsg=%s\n", rcStep, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    const char* sqlCount = "SELECT COUNT(*) FROM preguntas WHERE idCuestionario = ?;";
    for (int i = 0; i < count; i++) {
        sqlite3_stmt* stmtCount;
        if (sqlite3_prepare_v2(db, sqlCount, -1, &stmtCount, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtCount, 1, buffer[i].idCuestionario);
            buffer[i].cantPreguntas = (sqlite3_step(stmtCount) == SQLITE_ROW)
                                       ? sqlite3_column_int(stmtCount, 0) : 0;
            sqlite3_finalize(stmtCount);
        } else {
            buffer[i].cantPreguntas = 0;
        }
    }

    const char* sqlMateria = "SELECT materia FROM usuarios WHERE idUsuario = ?;";
    for (int i = 0; i < count; i++) {
        sqlite3_stmt* stmtMat;
        if (sqlite3_prepare_v2(db, sqlMateria, -1, &stmtMat, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtMat, 1, _idUsuarioTemp[i]);
            if (sqlite3_step(stmtMat) == SQLITE_ROW) {
                const char* materia = (const char*)sqlite3_column_text(stmtMat, 0);
                buffer[i].materia = materia ? String(materia) : "";
            } else {
                buffer[i].materia = "";
            }
            sqlite3_finalize(stmtMat);
        } else {
            buffer[i].materia = "";
        }
    }

    _logHeapRepo("LISTAR-TUTOR post");
    return count;
}