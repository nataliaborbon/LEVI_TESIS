#include "DatabaseManager.h"

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

bool DatabaseManager::_configurarPragmas()
{
    if (!_ejecutar("PRAGMA foreign_keys = ON;", "foreign_keys"))
        return false;
    if (!_ejecutar("PRAGMA journal_mode = WAL;", "journal_mode"))
        return false;
    if (!_ejecutar("PRAGMA synchronous = NORMAL;", "synchronous"))
        return false;
    return true;
}

bool DatabaseManager::_ejecutar(const char *sql, const char *descripcion)
{
    char *errMsg = nullptr;
    int rc = sqlite3_exec(_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        Serial.printf("[DB] Error en '%s': %s\n", descripcion, errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}
