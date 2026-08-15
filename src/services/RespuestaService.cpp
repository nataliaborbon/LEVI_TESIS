#include "services/RespuestaService.h"
#include "services/CuestionarioService.h"
#include "storage/database/repositories/CuestionarioRepository.h"
#include "storage/database/repositories/PreguntaOpcionRepository.h"
#include <string.h>

SesionResult RespuestaService::iniciarSesion() {
    return SessionManager::getInstance().iniciarSesionAlumno();
}

EstadoAlumno RespuestaService::obtenerEstado() {
    EstadoAlumno estado = _calcularEstado();
    _actualizarResumenCache(estado);
    return estado;
}

void RespuestaService::_actualizarResumenCache(const EstadoAlumno& estado) {
    strncpy(_resumenCache.estado, estado.estado.c_str(), sizeof(_resumenCache.estado) - 1);
    _resumenCache.estado[sizeof(_resumenCache.estado) - 1] = '\0';
    _resumenCache.numeroPregunta = estado.hayPregunta ? estado.pregunta.numeroPregunta : 0;
    _resumenCache.totalPreguntas = estado.hayPregunta ? estado.pregunta.totalPreguntas : 0;
    strncpy(_resumenCache.tituloCuestionario, estado.tituloCuestionario.c_str(), sizeof(_resumenCache.tituloCuestionario) - 1);
    _resumenCache.tituloCuestionario[sizeof(_resumenCache.tituloCuestionario) - 1] = '\0';
}

EstadoAlumno RespuestaService::_calcularEstado() {
    EstadoAlumno estado;

    const SesionPanel& panel = SessionManager::getInstance().getSesionPanel();
    if (panel.activa && panel.rol == "invitado") {
        const PreguntaInvitado& pregInv = SessionManager::getInstance().getPreguntaInvitado();
        if (pregInv.cargada) {
            estado.estado      = "invitado";
            estado.hayPregunta = true;
            estado.tituloCuestionario = "Modo Invitado";

            estado.pregunta.idPregunta     = 0;
            estado.pregunta.textoPregunta  = pregInv.textoOpregunta;
            estado.pregunta.numeroPregunta = 1;
            estado.pregunta.totalPreguntas = 1;
            estado.pregunta.cantOpciones   = pregInv.cantOpciones;

            for (int i = 0; i < pregInv.cantOpciones; i++) {
                estado.pregunta.opciones[i].idOpcion = i;
                estado.pregunta.opciones[i].opcion   = pregInv.opciones[i];
            }
            return estado;
        }
        estado.estado = "esperando";
        return estado;
    }

    // obtenerActivo() filtra WHERE estado='en_progreso', así que si devuelve
    // algo, ya sabemos que ese es el único estado posible. No hace falta
    // (ni tiene sentido) volver a comparar activo.estado acá.
    Cuestionario activo = CuestionarioRepository::getInstance().obtenerActivo();

    if (activo.idCuestionario == 0) {
        estado.estado = "esperando";
        return estado;
    }

    estado.tituloCuestionario = activo.titulo;
    estado.estado = "en_progreso";

    PreguntaAlumno pregAlumno;
    bool hay = PreguntaRepository::getInstance()
               .obtenerSiguienteParaAlumno(activo.idCuestionario, pregAlumno);

    if (!hay) {
        estado.hayPregunta = false;
        return estado;
    }

    pregAlumno.cantOpciones = OpcionRepository::getInstance()
                              .listarParaAlumno(pregAlumno.idPregunta,
                                                pregAlumno.opciones, 4);

    estado.hayPregunta = true;
    estado.pregunta    = pregAlumno;
    return estado;
}

RespuestaResult RespuestaService::responder(int idPregunta, int idOpcion) {
    RespuestaResult result;

    const SesionPanel& panel = SessionManager::getInstance().getSesionPanel();
    if (panel.activa && panel.rol == "invitado") {
        const PreguntaInvitado& pregInv = SessionManager::getInstance().getPreguntaInvitado();
        if (!pregInv.cargada || idOpcion < 0 || idOpcion >= pregInv.cantOpciones) {
            result.mensaje = "Opción inválida.";
            return result;
        }
        result.ok       = true;
        result.finalizo = true;
        result.mensaje  = pregInv.opciones[idOpcion];
        SessionManager::getInstance().limpiarPreguntaInvitado();
        return result;
    }

    Pregunta p = PreguntaRepository::getInstance().buscarPorId(idPregunta);
    if (p.idPregunta == 0) {
        result.mensaje = "Pregunta no encontrada.";
        return result;
    }

    Cuestionario activo = CuestionarioRepository::getInstance().obtenerActivo();
    if (activo.idCuestionario == 0 || p.idCuestionario != activo.idCuestionario) {
        result.mensaje = "No hay un examen activo o la pregunta no pertenece al mismo.";
        return result;
    }

    Opcion opc = OpcionRepository::getInstance().buscarPorId(idOpcion);
    if (opc.idOpcion == 0 || opc.idPregunta != idPregunta) {
        result.mensaje = "Opción inválida.";
        return result;
    }

    DbResult db = PreguntaRepository::getInstance()
                  .guardarRespuesta(idPregunta, idOpcion);
    if (!db.ok) {
        result.mensaje = "Error al guardar la respuesta.";
        return result;
    }

    result.ok          = true;
    result.fueCorrecto = (idOpcion == p.idOpcionCorrecta);

    int total      = PreguntaRepository::getInstance().contarTotal(activo.idCuestionario);
    int respondidas= PreguntaRepository::getInstance().contarRespondidas(activo.idCuestionario);
    result.finalizo = (respondidas >= total);

    if (result.finalizo) {
        CuestionarioResult finalRes = CuestionarioService::getInstance()
                                       .finalizarComoAlumno(activo.idCuestionario);
        if (finalRes.ok) {
            result.resultado = finalRes.resultado;
            result.aprobado  = result.resultado.puntajeObtenido >= result.resultado.puntajeParaAprobar;
        } else {
            // No debería pasar (ya verificamos respondidas >= total arriba),
            // pero si falla no rompemos la respuesta de "responder" en sí:
            // el alumno ya respondió, eso ya quedó guardado.
            Serial.printf("[RESPONDER] Error al finalizar: %s\n", finalRes.mensaje.c_str());
        }
    }

    return result;
}