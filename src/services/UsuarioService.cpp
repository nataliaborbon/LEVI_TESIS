#include "services/UsuarioService.h"
#include "services/AuthService.h"
#include "config/ConfigManager.h"
#include "storage/database/repositories/UsuarioRepository.h"

// ---------------------------------------------------------------------------
// Validación de campos
// ---------------------------------------------------------------------------

String UsuarioService::_validarCampos(const Usuario& u) {
    if (u.usuario.length() < 4 || u.usuario.length() > 30)
        return "El nombre de usuario debe tener entre 4 y 30 caracteres.";

    if (u.usuario.indexOf(' ') >= 0)
        return "El nombre de usuario no puede contener espacios.";

    if (u.nombre.length() == 0)
        return "El nombre es obligatorio.";

    if (u.apellido.length() == 0)
        return "El apellido es obligatorio.";

    if (u.contacto.length() == 0)
        return "El contacto es obligatorio.";

    if (u.rol == "profesor" && u.materia.length() == 0)
        return "La materia es obligatoria para profesores.";

    return "";
}

// ---------------------------------------------------------------------------
// Crear usuario
// ---------------------------------------------------------------------------

UsuarioResult UsuarioService::crear(Usuario& u, const String& password,
                                     const String& confirmar,
                                     const String& claveMaestra) {
    UsuarioResult result;

    // Verificar clave maestra según el rol a crear
    if (!ConfigManager::getInstance().verificarClavePorRol(claveMaestra, u.rol)) {
        result.mensaje = "Clave maestra incorrecta.";
        return result;
    }

    // Validar campos básicos
    String error = _validarCampos(u);
    if (error.length() > 0) {
        result.mensaje = error;
        return result;
    }

    // Verificar que las contraseñas coincidan
    if (password.length() == 0) {
        result.mensaje = "La contraseña es obligatoria.";
        return result;
    }
    if (password != confirmar) {
        result.mensaje = "Las contraseñas no coinciden.";
        return result;
    }

    // Verificar que el nombre de usuario no exista
    if (UsuarioRepository::getInstance().existeUsuario(u.usuario)) {
        result.mensaje = "El nombre de usuario ya está en uso.";
        return result;
    }

    // Hashear contraseña
    u.salt         = AuthService::getInstance().generarSalt();
    u.hashPassword = AuthService::getInstance().hashPassword(password, u.salt);

    // Persistir
    DbResult db = UsuarioRepository::getInstance().crear(u);
    if (!db.ok) {
        result.mensaje = "Error al crear el usuario: " + db.mensaje;
        return result;
    }

    result.ok = true;
    result.id = db.id;
    return result;
}

// ---------------------------------------------------------------------------
// Editar perfil
// ---------------------------------------------------------------------------

UsuarioResult UsuarioService::editarPerfil(const Usuario& u) {
    UsuarioResult result;

    // Validar campos básicos
    String error = _validarCampos(u);
    if (error.length() > 0) {
        result.mensaje = error;
        return result;
    }

    // Verificar que el nuevo nombre de usuario no esté tomado por otro
    if (UsuarioRepository::getInstance().existeUsuarioExcluyendo(u.usuario, u.idUsuario)) {
        result.mensaje = "El nombre de usuario ya está en uso.";
        return result;
    }

    DbResult db = UsuarioRepository::getInstance().actualizar(u);
    if (!db.ok) {
        result.mensaje = "Error al actualizar el perfil: " + db.mensaje;
        return result;
    }

    result.ok = true;
    result.id = u.idUsuario;
    return result;
}

// ---------------------------------------------------------------------------
// Recuperar contraseña
// ---------------------------------------------------------------------------

UsuarioResult UsuarioService::recuperarPassword(String usuario, String claveMaestra, String nueva, String confirmar)
{
    UsuarioResult resultado;
    resultado.ok = false;

    if (nueva != confirmar)
    {
        resultado.mensaje = "Las contraseñas no coinciden.";
        return resultado;
    }

    // 1. Buscamos al usuario usando tu repositorio
    Usuario u = UsuarioRepository::getInstance().buscarPorUsuario(usuario);
    if (u.idUsuario == 0)
    {
        resultado.mensaje = "El usuario no existe.";
        return resultado;
    }

    // 2. Verificamos la clave maestra usando tu ConfigManager
    if (!ConfigManager::getInstance().verificarClavePorRol(claveMaestra, u.rol)) {
        resultado.mensaje = "Clave maestra incorrecta.";
        return resultado;
    }

    if (nueva.length() == 0) {
        resultado.mensaje = "La nueva contraseña no puede estar vacía.";
        return resultado;
    }

    // 3. Generamos el nuevo Salt y el Hash usando tu AuthService
    String nuevoSalt = AuthService::getInstance().generarSalt();
    String nuevoHash = AuthService::getInstance().hashPassword(nueva, nuevoSalt);
    
    // 4. Guardamos los cambios usando la función ESPECÍFICA de tu repositorio
    DbResult db = UsuarioRepository::getInstance().actualizarPassword(u.idUsuario, nuevoHash, nuevoSalt);
    
    if (db.ok)
    {
        resultado.ok = true;
        resultado.mensaje = "Contraseña cambiada con éxito.";
    }
    else
    {
        resultado.mensaje = "Error interno al guardar: " + db.mensaje;
    }

    return resultado;
}

// ---------------------------------------------------------------------------
// Cambiar contraseña
// ---------------------------------------------------------------------------
UsuarioResult UsuarioService::cambiarPassword(int idUsuario, const String& actual, const String& nueva, const String& confirmar) {
    UsuarioResult resultado;
    resultado.ok = false;

    if (nueva != confirmar) {
        resultado.mensaje = "Las contraseñas no coinciden.";
        return resultado;
    }

    // Buscamos al usuario por su ID (ya sabemos que está logueado)
    Usuario u = UsuarioRepository::getInstance().buscarPorId(idUsuario);
    if (u.idUsuario == 0) {
        resultado.mensaje = "Usuario no encontrado.";
        return resultado;
    }

    // Hasheamos la contraseña actual que ingresó y la comparamos con la guardada
    String hashActual = AuthService::getInstance().hashPassword(actual, u.salt);
    if (hashActual != u.hashPassword) {
        resultado.mensaje = "La contraseña actual es incorrecta.";
        return resultado;
    }

    if (nueva.length() == 0) {
        resultado.mensaje = "La nueva contraseña no puede estar vacía.";
        return resultado;
    }

    // Si pasó la prueba de seguridad, generamos la nueva
    String nuevoSalt = AuthService::getInstance().generarSalt();
    String nuevoHash = AuthService::getInstance().hashPassword(nueva, nuevoSalt);

    DbResult db = UsuarioRepository::getInstance().actualizarPassword(idUsuario, nuevoHash, nuevoSalt);

    if (db.ok) {
        resultado.ok = true;
        resultado.mensaje = "Contraseña cambiada con éxito.";
    } else {
        resultado.mensaje = "Error interno al guardar: " + db.mensaje;
    }

    return resultado;
}

// ---------------------------------------------------------------------------
// Eliminar usuario
// ---------------------------------------------------------------------------
UsuarioResult UsuarioService::eliminar(int idUsuario) {
    UsuarioResult result;

    // Buscamos el usuario por su ID para conocer su rol
    Usuario u = UsuarioRepository::getInstance().buscarPorId(idUsuario);
    if (u.idUsuario == 0) {
        result.ok = false;
        result.mensaje = "Usuario no encontrado en el sistema.";
        return result;
    }

    DbResult db = UsuarioRepository::getInstance().eliminar(idUsuario);
    if (!db.ok) {
        result.ok = false;
        result.mensaje = "Error al eliminar el usuario: " + db.mensaje;
        return result;
    }

    result.ok = true;
    return result;
}

// ---------------------------------------------------------------------------
// Obtener perfil
// ---------------------------------------------------------------------------

Usuario UsuarioService::obtenerPerfil(int idUsuario) {
    return UsuarioRepository::getInstance().buscarPorId(idUsuario);
}

// ---------------------------------------------------------------------------
// Listados
// ---------------------------------------------------------------------------

int UsuarioService::listarProfesores(ProfesorResumen* buffer, int maxSize) {
    return UsuarioRepository::getInstance().listarProfesores(buffer, maxSize);
}

int UsuarioService::listarTutores(TutorResumen* buffer, int maxSize) {
    return UsuarioRepository::getInstance().listarTutores(buffer, maxSize);
}
