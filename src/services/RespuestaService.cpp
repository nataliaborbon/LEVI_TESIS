#include "RespuestaService.h"
#include "../storage/database/repositories/CuestionarioRepository.h"
#include "../storage/database/repositories/PreguntaOpcionRepository.h"

SesionResult RespuestaService::iniciarSesion() {
    return SessionManager::getInstance().iniciarSesionAlumno();
}

EstadoAlumno RespuestaService::obtenerEstado() {
    EstadoAlumno estado;

    // Verificar primero si hay un invitado activo con pregunta en RAM
    const SesionPanel& panel = SessionManager::getInstance().getSesionPanel();
    if (panel.activa && panel.rol == "invitado") {
        const PreguntaInvitado& pregInv = SessionManager::getInstance().getPreguntaInvitado();
        if (pregInv.cargada) {
            estado.estado      = "invitado";
            estado.hayPregunta = true;

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

    // Buscar cuestionario activo en BD
    Cuestionario activo = CuestionarioRepository::getInstance().obtenerActivo();

    if (activo.idCuestionario == 0) {
        estado.estado = "esperando";
        return estado;
    }

    if (activo.estado == "pausado") {
        estado.estado = "pausado";
        return estado;
    }

    if (activo.estado == "finalizado") {
        estado.estado             = "finalizado";
        estado.puntajeObtenido    = activo.puntajeObtenido;
        estado.puntajeParaAprobar = activo.puntajeParaAprobar;
        estado.aprobado           = activo.puntajeObtenido >= activo.puntajeParaAprobar;
        estado.tiempoSegundos     = activo.tiempoSegundos;
        return estado;
    }

    // Estado en_progreso: buscar siguiente pregunta sin responder
    estado.estado = "en_progreso";

    PreguntaAlumno pregAlumno;
    bool hay = PreguntaRepository::getInstance()
               .obtenerSiguienteParaAlumno(activo.idCuestionario, pregAlumno);

    if (!hay) {
        // Todas respondidas pero no finalizado aún (caso borde)
        estado.hayPregunta = false;
        return estado;
    }

    // Cargar opciones sin revelar la correcta
    pregAlumno.cantOpciones = OpcionRepository::getInstance()
                              .listarParaAlumno(pregAlumno.idPregunta,
                                                pregAlumno.opciones, 4);

    estado.hayPregunta = true;
    estado.pregunta    = pregAlumno;
    return estado;
}

RespuestaResult RespuestaService::responder(int idPregunta, int idOpcion) {
    RespuestaResult result;

    // Modo invitado: sin BD
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
        return result;
    }

    // Modo normal: verificar que la pregunta existe y pertenece al cuestionario activo
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

    // Verificar que la opción pertenece a la pregunta
    Opcion opc = OpcionRepository::getInstance().buscarPorId(idOpcion);
    if (opc.idOpcion == 0 || opc.idPregunta != idPregunta) {
        result.mensaje = "Opción inválida.";
        return result;
    }

    // Guardar respuesta
    DbResult db = PreguntaRepository::getInstance()
                  .guardarRespuesta(idPregunta, idOpcion);
    if (!db.ok) {
        result.mensaje = "Error al guardar la respuesta.";
        return result;
    }

    result.ok          = true;
    result.fueCorrecto = (idOpcion == p.idOpcionCorrecta);

    // Verificar si era la última pregunta sin responder
    int total      = PreguntaRepository::getInstance().contarTotal(activo.idCuestionario);
    int respondidas= PreguntaRepository::getInstance().contarRespondidas(activo.idCuestionario);
    result.finalizo = (respondidas >= total);

    return result;
}
