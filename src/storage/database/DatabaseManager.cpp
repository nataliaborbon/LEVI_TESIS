#include "storage/database/DatabaseManager.h"

// ---------------------------------------------------------------------------
// Heap estático para SQLite — reservado en BSS en tiempo de compilación.
// SQLite usa este bloque exclusivamente y nunca compite con el heap dinámico.
// 24KB cubre el INSERT más grande del sistema (20 preguntas × 4 opciones).
// ---------------------------------------------------------------------------
static uint8_t _sqliteHeap[24576];

bool DatabaseManager::begin(const char *rutaArchivo)
{
    // Configurar heap estático ANTES de abrir la BD.
    // El tercer parámetro (64) es el tamaño mínimo de alocación interna.
    //sqlite3_config(SQLITE_CONFIG_HEAP, _sqliteHeap, sizeof(_sqliteHeap), 64);

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
    // foreign_keys: integridad referencial entre tablas
    if (!_ejecutar("PRAGMA foreign_keys = ON;", "foreign_keys"))
        return false;

    // journal_mode DELETE: más simple que WAL, sin archivo de checkpoint separado.
    // Elimina el problema de SELECT que devuelve datos desactualizados después
    // de un DELETE/INSERT, que ocurría porque WAL no había hecho checkpoint aún.
    if (!_ejecutar("PRAGMA journal_mode = DELETE;", "journal_mode"))
        return false;

    // synchronous NORMAL: balance entre seguridad y velocidad.
    // FULL sería más seguro pero más lento en SD.
    if (!_ejecutar("PRAGMA synchronous = NORMAL;", "synchronous"))
        return false;

    // cache_size: 2KB de caché de páginas en el heap estático de SQLite.
    // Negativo = kilobytes, positivo = páginas.
    if (!_ejecutar("PRAGMA cache_size = -2;", "cache_size"))
        return false;

    // temp_store MEMORY: tablas temporales en RAM en lugar de SD.
    // Acelera operaciones con ORDER BY, GROUP BY, y subqueries.
    if (!_ejecutar("PRAGMA temp_store = MEMORY;", "temp_store"))
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