#include "storage/database/DatabaseManager.h"

bool DatabaseManager::begin(const char *rutaArchivo)
{
    int rc = sqlite3_open(rutaArchivo, &_db);

    if (rc != SQLITE_OK)
    {
        Serial.printf("[DB] Error al abrir BD: %s\n", sqlite3_errmsg(_db));
        _db = nullptr;
        return false;
    }

    Serial.printf("[DB] BD abierta: %s\n", rutaArchivo);

    if (!_configurarPragmas())
    {
        end();
        return false;
    }

    Serial.println("[DB] Lista.");
    return true;
}


void DatabaseManager::end()
{
    if (_db)
    {
        sqlite3_close(_db);
        _db = nullptr;
        Serial.println("[DB] Conexión cerrada.");
    }
}


// ---------------------------------------------------------------------------
// Configuración SQLite
// ---------------------------------------------------------------------------

bool DatabaseManager::_configurarPragmas()
{
    if (!_ejecutar(
        "PRAGMA foreign_keys = ON;",
        "foreign_keys"))
        return false;

    if (!_ejecutar(
        "PRAGMA journal_mode = PERSIST;",
        "journal_mode"))
        return false;

    if (!_ejecutar(
        "PRAGMA synchronous = NORMAL;",
        "synchronous"))
        return false;

    if (!_ejecutar(
        "PRAGMA cache_size = -16;",
        "cache_size"))
        return false;

    if (!_ejecutar(
        "PRAGMA temp_store = FILE;",
        "temp_store"))
        return false;

    if (!_ejecutar(
        "PRAGMA busy_timeout = 3000;",
        "busy_timeout"))
        return false;


    return true;
}


// ---------------------------------------------------------------------------
// Ejecución simple de SQL
// ---------------------------------------------------------------------------

bool DatabaseManager::_ejecutar(const char *sql, const char *descripcion)
{
    char *errMsg = nullptr;

    int rc = sqlite3_exec(
        _db,
        sql,
        nullptr,
        nullptr,
        &errMsg
    );


    if (rc != SQLITE_OK)
    {
        Serial.printf(
            "[DB] Error en '%s': %s\n",
            descripcion,
            errMsg ? errMsg : "desconocido"
        );

        sqlite3_free(errMsg);
        return false;
    }

    return true;
}


// ---------------------------------------------------------------------------
// Transacciones
// ---------------------------------------------------------------------------

bool DatabaseManager::beginTransaction()
{
    Serial.println("[DB] BEGIN TRANSACTION");

    return _ejecutar(
        "BEGIN TRANSACTION;",
        "begin_transaction"
    );
}


bool DatabaseManager::commit()
{
    Serial.println("[DB] COMMIT");

    return _ejecutar(
        "COMMIT;",
        "commit"
    );
}


bool DatabaseManager::rollback()
{
    Serial.println("[DB] ROLLBACK");

    char *errMsg = nullptr;

    int rc = sqlite3_exec(
        _db,
        "ROLLBACK;",
        nullptr,
        nullptr,
        &errMsg
    );

    if (rc != SQLITE_OK)
    {
        Serial.printf(
            "[DB] rollback no realizado: %s\n",
            errMsg ? errMsg : "desconocido"
        );

        sqlite3_free(errMsg);
        return false;
    }

    return true;
}