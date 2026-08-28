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

    // Si hay una finalización reciente "congelada" (ver _registrarFinalizacion),
    // no dejamos que el cálculo normal (que acá ya da "esperando", porque el
    // cuestionario dejó de estar en_progreso) pise el resumen que le estamos
    // mostrando al CYD. El EstadoAlumno que se devuelve al navegador NO se ve
    // afectado por esto: sigue siendo el cálculo real de siempre.
    if ((long)(millis() - _finalizadoHastaMs) >= 0) {
        _actualizarResumenCache(estado);
    }

    // NUEVO: El reloj solo avanza cuando el front consulta el estado (cada 2 segundos)
    if (estado.estado == "en_progreso") {
        Cuestionario activo = CuestionarioRepository::getInstance().obtenerActivo();
        if (activo.idCuestionario != 0) {
            CuestionarioService::getInstance().procesarHeartbeatCronometro(activo.idCuestionario);
        }
    }

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

void RespuestaService::_registrarFinalizacion(const ResultadoFinalizacion& resultado, bool aprobado, const String& titulo) {
    strncpy(_resumenCache.estado, "finalizado", sizeof(_resumenCache.estado) - 1);
    _resumenCache.estado[sizeof(_resumenCache.estado) - 1] = '\0';
    strncpy(_resumenCache.tituloCuestionario, titulo.c_str(), sizeof(_resumenCache.tituloCuestionario) - 1);
    _resumenCache.tituloCuestionario[sizeof(_resumenCache.tituloCuestionario) - 1] = '\0';
    _resumenCache.numeroPregunta    = 0;
    _resumenCache.totalPreguntas    = 0;
    _resumenCache.puntajeObtenido    = resultado.puntajeObtenido;
    _resumenCache.puntajeParaAprobar = resultado.puntajeParaAprobar;
    _resumenCache.puntajeMaximo      = resultado.puntajeMaximo;
    _resumenCache.tiempoSegundos     = resultado.tiempoSegundos;
    _resumenCache.aprobado           = aprobado;

    // Un poco más larga que los 10s que el CYD muestra el resultado, para
    // darle margen al loop() de 1s y no cortar la vista a mitad de camino.
    _finalizadoHastaMs = millis() + 15000UL;
}

EstadoAlumno RespuestaService::_calcularEstado() {
    EstadoAlumno estado;

    bool hayOficialEnCurso = CuestionarioRepository::getInstance().hayUnoEnProgreso();

    const SesionPanel& panel = SessionManager::getInstance().getSesionPanel();
    if (!hayOficialEnCurso && panel.activa && panel.rol == "invitado") {
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
            _registrarFinalizacion(result.resultado, result.aprobado, activo.titulo);
        } else {
            // No debería pasar (ya verificamos respondidas >= total arriba),
            // pero si falla no rompemos la respuesta de "responder" en sí:
            // el alumno ya respondió, eso ya quedó guardado.
            Serial.printf("[RESPONDER] Error al finalizar: %s\n", finalRes.mensaje.c_str());
        }
    }

    return result;
}