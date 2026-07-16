#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @file ConfigManager.h
 * @brief Gestión de las claves maestras del sistema via NVS.
 *
 * Almacena dos claves maestras en la memoria NVS (Non-Volatile Storage)
 * interna de la ESP32, inaccesible extrayendo la tarjeta SD.
 *
 * Clave de profesor: permite crear y borrar usuarios con rol 'profesor'.
 * Clave de tutor:    permite crear y borrar usuarios con rol 'tutor'.
 *
 * Ambas se configuran en el primer arranque desde el teclado en pantalla del CYD.
 */
class ConfigManager {
public:
    /**
     * @brief Devuelve la instancia única (Singleton).
     * @return Referencia a la instancia de ConfigManager.
     */
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    // -----------------------------------------------------------------------
    // Estado de configuración
    // -----------------------------------------------------------------------

    /**
     * @brief Indica si ambas claves maestras ya fueron configuradas.
     * @return true si tanto la clave de profesor como la de tutor existen en NVS.
     */
    bool clavesConfiguradas();

    /**
     * @brief Indica si la clave de profesor fue configurada.
     * @return true si existe en NVS.
     */
    bool claveProfesorConfigurada();

    /**
     * @brief Indica si la clave de tutor fue configurada.
     * @return true si existe en NVS.
     */
    bool claveTutorConfigurada();

    // -----------------------------------------------------------------------
    // Guardar claves (primer arranque)
    // -----------------------------------------------------------------------

    /**
     * @brief Guarda la clave maestra para profesores en NVS.
     * @param clave Clave a guardar. No puede ser vacía.
     * @return true si se guardó correctamente.
     */
    bool guardarClaveProfesor(const String& clave);

    /**
     * @brief Guarda la clave maestra para tutores en NVS.
     * @param clave Clave a guardar. No puede ser vacía.
     * @return true si se guardó correctamente.
     */
    bool guardarClaveTutor(const String& clave);

    // -----------------------------------------------------------------------
    // Verificar claves (en cada operación)
    // -----------------------------------------------------------------------

    /**
     * @brief Verifica si una clave coincide con la clave maestra de profesores.
     * @param clave Clave a verificar.
     * @return true si coincide.
     */
    bool verificarClaveProfesor(const String& clave);

    /**
     * @brief Verifica si una clave coincide con la clave maestra de tutores.
     * @param clave Clave a verificar.
     * @return true si coincide.
     */
    bool verificarClaveTutor(const String& clave);

    /**
     * @brief Verifica la clave maestra correspondiente al rol dado.
     * @param clave Clave a verificar.
     * @param rol   "profesor" o "tutor".
     * @return true si la clave coincide con la del rol indicado.
     */
    bool verificarClavePorRol(const String& clave, const String& rol);

private:
    ConfigManager() {}
    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    /// @brief Namespace usado en NVS para aislar los datos de esta app.
    static constexpr const char* NVS_NAMESPACE      = "levi_cfg";

    /// @brief Clave NVS para la clave maestra de profesores.
    static constexpr const char* KEY_CLAVE_PROFESOR = "clave_prof";

    /// @brief Clave NVS para la clave maestra de tutores.
    static constexpr const char* KEY_CLAVE_TUTOR    = "clave_tutor";

    /**
     * @brief Lee una clave del NVS por su key interna.
     * @param key Key NVS a leer.
     * @return Valor almacenado, o cadena vacía si no existe.
     */
    String _leer(const char* key);

    /**
     * @brief Guarda un valor en NVS.
     * @param key   Key NVS destino.
     * @param valor Valor a guardar.
     * @return true si se guardó correctamente.
     */
    bool _guardar(const char* key, const String& valor);
};

#endif // CONFIG_MANAGER_H
