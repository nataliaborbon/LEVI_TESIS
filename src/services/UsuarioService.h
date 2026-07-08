#ifndef USUARIO_SERVICE_H
#define USUARIO_SERVICE_H

#include <Arduino.h>
#include "../storage/models/Models.h"

/**
 * @file UsuarioService.h
 * @brief Lógica de negocio para la gestión de usuarios.
 *
 * Responsabilidades:
 *   - Validar datos antes de crear o editar un usuario.
 *   - Verificar la clave maestra correspondiente al rol.
 *   - Hashear contraseñas antes de pasarlas al repository.
 *   - Coordinar UsuarioRepository y AuthService.
 */

/**
 * @brief Resultado estándar de operaciones de usuario.
 */
struct UsuarioResult {
    bool   ok      = false;
    String mensaje = "";
    int    id      = 0;
};

class UsuarioService {
public:
    /**
     * @brief Devuelve la instancia única.
     * @return Referencia al UsuarioService.
     */
    static UsuarioService& getInstance() {
        static UsuarioService instance;
        return instance;
    }

    /**
     * @brief Crea un usuario nuevo previa verificación de la clave maestra.
     *
     * Valida:
     *   - Que la clave maestra corresponda al rol a crear.
     *   - Que el nombre de usuario no exista ya.
     *   - Que usuario, nombre, apellido y contacto no estén vacíos.
     *   - Que el usuario no tenga espacios y tenga entre 4 y 30 caracteres.
     *   - Que si el rol es 'profesor', materia no esté vacía.
     *   - Que las contraseñas coincidan.
     *
     * @param u            Usuario a crear (sin hashPassword ni salt, se calculan acá).
     * @param password     Contraseña en texto plano.
     * @param confirmar    Confirmación de la contraseña.
     * @param claveMaestra Clave maestra del rol correspondiente.
     * @return UsuarioResult con ok=true e id generado si tuvo éxito.
     */
    UsuarioResult crear(Usuario& u, const String& password,
                        const String& confirmar, const String& claveMaestra);

    /**
     * @brief Edita el perfil del usuario logueado.
     *
     * Valida:
     *   - Que el nuevo nombre de usuario no exista ya (si cambió).
     *   - Que usuario, nombre, apellido y contacto no estén vacíos.
     *   - Que el usuario no tenga espacios y tenga entre 4 y 30 caracteres.
     *   - Que si el rol es 'profesor', materia no esté vacía.
     *
     * @param u Usuario con los datos nuevos (idUsuario debe ser válido).
     * @return UsuarioResult con ok=true si tuvo éxito.
     */
    UsuarioResult editarPerfil(const Usuario& u);

    /**
     * @brief Recupera la contraseña de un usuario cualquiera
     *
     * Valida:
     *   - Que la palabra clave sea correcta
     *   - Que la nueva contraseña y su confirmación coincidan.
     *   - Que la nueva contraseña no esté vacía.
     *
     * @param usuario         Nombre de usuario
     * @param claveMaestra    Clave maestra del administrador
     * @param nueva           Nueva contraseña en texto plano.
     * @param confirmar       Confirmación de la nueva contraseña.
     * @return UsuarioResult con ok=true si tuvo éxito.
     */
    UsuarioResult recuperarPassword(String usuario, String claveMaestra, String nueva, String confirmar);

    /**
     * @brief
     * 
     * Valida:
     *   - Que la contraseña actual sea correcta.
     *   - Que la nueva contraseña y su confirmación coincidan.
     *   - Que la nueva contraseña no esté vacía.
     * 
     * @param idUsuario       ID del usuario
     * @param actual          Contraseña actual en texto plano
     * @param nueva           Nueva contraseña en texto plano.
     * @param confirmar       Confirmación de la nueva contraseña.
     * @return UsuarioResult con ok=true si tuvo éxito.
     */
    UsuarioResult cambiarPassword(int idUsuario, const String& actual, const String& nueva, const String& confirmar);

    /**
     * @brief Elimina un usuario por numero de ID
     * 
     * Valida:
     *  -Que el usuario exista
     *
     * 
     *
     * @param idUsuario Numero ID del usuario a eliminar
     * @return UsuarioResult con ok=true si tuvo éxito.
     */
    UsuarioResult eliminar(int idUsuario);

    /**
     * @brief Obtiene el perfil completo de un usuario por id.
     * @param idUsuario Id del usuario.
     * @return Usuario encontrado, o struct con idUsuario=0 si no existe.
     */
    Usuario obtenerPerfil(int idUsuario);

    /**
     * @brief Lista los profesores en formato resumido.
     * @param buffer  Array destino.
     * @param maxSize Capacidad máxima del array.
     * @return Cantidad de profesores encontrados.
     */
    int listarProfesores(ProfesorResumen* buffer, int maxSize);

    /**
     * @brief Lista los tutores en formato resumido.
     * @param buffer  Array destino.
     * @param maxSize Capacidad máxima del array.
     * @return Cantidad de tutores encontrados.
     */
    int listarTutores(TutorResumen* buffer, int maxSize);

private:
    UsuarioService() {}
    UsuarioService(const UsuarioService&)            = delete;
    UsuarioService& operator=(const UsuarioService&) = delete;

    /**
     * @brief Valida los campos básicos de un usuario.
     * @param u Usuario a validar.
     * @return Mensaje de error, o cadena vacía si todo está bien.
     */
    String _validarCampos(const Usuario& u);
};



#endif // USUARIO_SERVICE_H
