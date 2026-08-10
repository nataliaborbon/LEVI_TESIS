#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <Arduino.h>
#include <sqlite3.h>

/**
 * @file DatabaseManager.h
 * @brief Gestión de la conexión con la base de datos SQLite.
 *
 * Singleton encargado de abrir la base SQLite, configurar PRAGMAs
 * y proveer la conexión activa a los repositories.
 *
 * Los repositories obtienen la conexión mediante getDB().
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
     *
     * @param rutaArchivo Ruta VFS del archivo SQLite.
     * @return true si la conexión se estableció correctamente.
     */
    bool begin(const char *rutaArchivo);


    /**
     * @brief Cierra la conexión con la base de datos.
     */
    void end();


    /**
     * @brief Devuelve el puntero a la conexión activa.
     *
     * @return Puntero sqlite3*, usado por los repositories.
     */
    sqlite3 *getDB()
    {
        return _db;
    }


    /**
     * @brief Indica si la BD está abierta y lista para usarse.
     *
     * @return true si la conexión está activa.
     */
    bool isReady()
    {
        return _db != nullptr;
    }

    /**
    * @brief Ejecuta SQL arbitrario desde fuera del manager.
    * @param sql Sentencia SQL a ejecutar.
    * @return true si fue exitoso.
    */
    bool ejecutar(const char* sql) { return _ejecutar(sql, sql); }


    // -----------------------------------------------------------------------
    // Transacciones SQLite
    // -----------------------------------------------------------------------

    /**
     * @brief Inicia una transacción SQLite.
     *
     * Agrupa múltiples INSERT/UPDATE/DELETE en una única operación
     * de escritura sobre la memoria persistente.
     *
     * @return true si la transacción comenzó correctamente.
     */
    bool beginTransaction();


    /**
     * @brief Confirma una transacción activa.
     *
     * @return true si el COMMIT fue exitoso.
     */
    bool commit();


    /**
     * @brief Cancela una transacción activa.
     *
     * @return true si el ROLLBACK fue exitoso.
     */
    bool rollback();

    void logUsoHeapSqlite();



private:

    DatabaseManager() : _db(nullptr) {}
    ~DatabaseManager()
    {
        end();
    }


    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;


    sqlite3 *_db;


    /**
     * @brief Configura los PRAGMAs de SQLite.
     *
     * Activa:
     * - foreign_keys
     * - journal_mode WAL
     * - synchronous NORMAL
     * - cache optimizada
     * - temp_store MEMORY
     * - busy_timeout
     *
     * @return true si todos los PRAGMAs se aplicaron correctamente.
     */
    bool _configurarPragmas();


    /**
     * @brief Ejecuta SQL sin retorno de filas.
     *
     * @param sql Sentencia SQL a ejecutar.
     * @param descripcion Texto descriptivo para logs.
     *
     * @return true si la ejecución fue exitosa.
     */
    bool _ejecutar(
        const char *sql,
        const char *descripcion
    );
};


#endif // DATABASE_MANAGER_H