#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <Arduino.h>
#include <sqlite3.h>

/**
 * @file DatabaseManager.h
 * @brief Gestión de la conexión con la base de datos SQLite en la SD.
 *
 * Singleton que abre el archivo .db y configura los PRAGMAs necesarios.
 * Las tablas se asumen ya creadas en la SD de forma manual.
 *
 * Los repositories obtienen la conexión activa mediante getDB().
 */
class DatabaseManager
{
public:
    /**
     * @brief Devuelve la instancia única.
     * @return Referencia al DatabaseManager.
     */
    static DatabaseManager &getInstance()
    {
        static DatabaseManager instance;
        return instance;
    }

    /**
     * @brief Abre la BD y configura los PRAGMAs.
     * @param rutaArchivo Ruta VFS del archivo SQLite (ej: "/sd/levi.db")
     * @return true si la conexión se estableció correctamente.
     */
    bool begin(const char *rutaArchivo);

    /**
     * @brief Cierra la conexión con la base de datos.
     */
    void end();

    /**
     * @brief Devuelve el puntero a la conexión activa.
     * @return Puntero sqlite3*, usado por los repositories.
     */
    sqlite3 *getDB() { return _db; }

    /**
     * @brief Indica si la BD está abierta y lista para usarse.
     * @return true si la conexión está activa.
     */
    bool isReady() { return _db != nullptr; }

private:
    DatabaseManager() : _db(nullptr) {}
    ~DatabaseManager() { end(); }

    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    sqlite3 *_db;

    /**
     * @brief Activa foreign keys, WAL y synchronous NORMAL.
     * @return true si todos los PRAGMAs se aplicaron correctamente.
     */
    bool _configurarPragmas();

    /**
     * @brief Ejecuta SQL sin retorno de filas.
     * @param sql         Sentencia SQL a ejecutar.
     * @param descripcion Texto descriptivo para logs de error.
     * @return true si la ejecución fue exitosa.
     */
    bool _ejecutar(const char *sql, const char *descripcion);
};

#endif // DATABASE_MANAGER_H
