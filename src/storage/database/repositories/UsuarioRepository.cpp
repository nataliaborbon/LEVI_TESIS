#include "UsuarioRepository.h"

Usuario UsuarioRepository::_filaAUsuario(sqlite3_stmt* stmt) {
    Usuario u;
    u.idUsuario    = sqlite3_column_int(stmt, 0);
    u.usuario      = String((const char*)sqlite3_column_text(stmt, 1));
    u.nombre       = String((const char*)sqlite3_column_text(stmt, 2));
    u.apellido     = String((const char*)sqlite3_column_text(stmt, 3));
    u.rol          = String((const char*)sqlite3_column_text(stmt, 4));

    const char* materia = (const char*)sqlite3_column_text(stmt, 5);
    u.materia = materia ? String(materia) : "";

    u.contacto     = String((const char*)sqlite3_column_text(stmt, 6));
    u.hashPassword = String((const char*)sqlite3_column_text(stmt, 7));
    u.salt         = String((const char*)sqlite3_column_text(stmt, 8));
    return u;
}

DbResult UsuarioRepository::crear(const Usuario& u) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        INSERT INTO usuarios
            (usuario, nombre, apellido, rol, materia, contacto, hashPassword, salt)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_text(stmt, 1, u.usuario.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, u.nombre.c_str(),       -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, u.apellido.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, u.rol.c_str(),          -1, SQLITE_TRANSIENT);

    if (u.materia.length() > 0) {
        sqlite3_bind_text(stmt, 5, u.materia.c_str(),  -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 5);
    }

    sqlite3_bind_text(stmt, 6, u.contacto.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, u.hashPassword.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, u.salt.c_str(),         -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result.ok = true;
        result.id = (int)sqlite3_last_insert_rowid(db);
    } else {
        result.mensaje = String("step error: ") + sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);
    return result;
}

Usuario UsuarioRepository::buscarPorUsuario(const String& usuario) {
    Usuario u;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        SELECT idUsuario, usuario, nombre, apellido, rol, materia,
               contacto, hashPassword, salt
        FROM usuarios WHERE usuario = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return u;

    sqlite3_bind_text(stmt, 1, usuario.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) u = _filaAUsuario(stmt);

    sqlite3_finalize(stmt);
    return u;
}

Usuario UsuarioRepository::buscarPorId(int idUsuario) {
    Usuario u;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    const char* sql = R"(
        SELECT idUsuario, usuario, nombre, apellido, rol, materia,
               contacto, hashPassword, salt
        FROM usuarios WHERE idUsuario = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return u;

    sqlite3_bind_int(stmt, 1, idUsuario);

    if (sqlite3_step(stmt) == SQLITE_ROW) u = _filaAUsuario(stmt);

    sqlite3_finalize(stmt);
    return u;
}

DbResult UsuarioRepository::actualizar(const Usuario& u) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    // El rol no se puede editar, por eso no está en el SET.
    const char* sql = R"(
        UPDATE usuarios
        SET usuario = ?, nombre = ?, apellido = ?, materia = ?, contacto = ?
        WHERE idUsuario = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_text(stmt, 1, u.usuario.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, u.nombre.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, u.apellido.c_str(), -1, SQLITE_TRANSIENT);

    if (u.materia.length() > 0) {
        sqlite3_bind_text(stmt, 4, u.materia.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    sqlite3_bind_text(stmt, 5, u.contacto.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 6, u.idUsuario);

    result.ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!result.ok) result.mensaje = String("step error: ") + sqlite3_errmsg(db);

    sqlite3_finalize(stmt);
    return result;
}

DbResult UsuarioRepository::actualizarPassword(int idUsuario,
                                                const String& nuevoHash,
                                                const String& nuevoSalt) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db,
        "UPDATE usuarios SET hashPassword = ?, salt = ? WHERE idUsuario = ?;",
        -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_text(stmt, 1, nuevoHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, nuevoSalt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 3, idUsuario);

    result.ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!result.ok) result.mensaje = String("step error: ") + sqlite3_errmsg(db);

    sqlite3_finalize(stmt);
    return result;
}

DbResult UsuarioRepository::eliminar(int idUsuario) {
    DbResult result;
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM usuarios WHERE idUsuario = ?;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        result.mensaje = String("prepare error: ") + sqlite3_errmsg(db);
        return result;
    }

    sqlite3_bind_int(stmt, 1, idUsuario);

    result.ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!result.ok) result.mensaje = String("step error: ") + sqlite3_errmsg(db);

    sqlite3_finalize(stmt);
    return result;
}

bool UsuarioRepository::existeUsuario(const String& usuario) {
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT 1 FROM usuarios WHERE usuario = ? LIMIT 1;",
        -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, usuario.c_str(), -1, SQLITE_TRANSIENT);

    bool existe = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return existe;
}

bool UsuarioRepository::existeUsuarioExcluyendo(const String& usuario, int excluirId) {
    sqlite3* db = DatabaseManager::getInstance().getDB();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT 1 FROM usuarios WHERE usuario = ? AND idUsuario != ? LIMIT 1;",
        -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, usuario.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 2, excluirId);

    bool existe = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return existe;
}

int UsuarioRepository::listarProfesores(ProfesorResumen* buffer, int maxSize) {
    sqlite3* db = DatabaseManager::getInstance().getDB();
    int count = 0;

    const char* sql = R"(
        SELECT idUsuario, nombre, apellido, materia, contacto
        FROM usuarios
        WHERE rol = 'profesor'
        ORDER BY apellido, nombre;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    while (sqlite3_step(stmt) == SQLITE_ROW && count < maxSize) {
        ProfesorResumen& p = buffer[count++];
        p.idUsuario = sqlite3_column_int(stmt, 0);
        p.nombre    = String((const char*)sqlite3_column_text(stmt, 1));
        p.apellido  = String((const char*)sqlite3_column_text(stmt, 2));

        const char* materia = (const char*)sqlite3_column_text(stmt, 3);
        p.materia = materia ? String(materia) : "";

        p.contacto  = String((const char*)sqlite3_column_text(stmt, 4));
    }

    sqlite3_finalize(stmt);
    return count;
}

int UsuarioRepository::listarTutores(TutorResumen* buffer, int maxSize) {
    sqlite3* db = DatabaseManager::getInstance().getDB();
    int count = 0;

    const char* sql = R"(
        SELECT idUsuario, nombre, apellido, contacto
        FROM usuarios
        WHERE rol = 'tutor'
        ORDER BY apellido, nombre;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    while (sqlite3_step(stmt) == SQLITE_ROW && count < maxSize) {
        TutorResumen& t = buffer[count++];
        t.idUsuario = sqlite3_column_int(stmt, 0);
        t.nombre    = String((const char*)sqlite3_column_text(stmt, 1));
        t.apellido  = String((const char*)sqlite3_column_text(stmt, 2));
        t.contacto  = String((const char*)sqlite3_column_text(stmt, 3));
    }

    sqlite3_finalize(stmt);
    return count;
}
