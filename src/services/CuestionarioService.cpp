#include "services/CuestionarioService.h"
#include "storage/database/repositories/CuestionarioRepository.h"
#include "storage/database/repositories/PreguntaOpcionRepository.h"

// ---------------------------------------------------------------------------
// Helpers privados
// ---------------------------------------------------------------------------

bool CuestionarioService::_esDuenio(int idCuestionario, int idUsuario) {
    Cuestionario c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);
    return c.idCuestionario != 0 && c.idUsuario == idUsuario;
}

float CuestionarioService::_calcularPuntaje(int idCuestionario) {
    Pregunta preguntas[20];
    int cant = PreguntaRepository::getInstance()
                .listarPorCuestionario(idCuestionario, preguntas, 20);

    float puntaje = 0.0f;
    for (int i = 0; i < cant; i++) {
        if (preguntas[i].idOpcionElegida == 0) continue; // No respondida

        if (preguntas[i].idOpcionElegida == preguntas[i].idOpcionCorrecta) {
            puntaje += preguntas[i].puntajeCorrecta;
        } else {
            puntaje -= preguntas[i].puntajeIncorrecta;
        }
    }
    return puntaje;
}

// ---------------------------------------------------------------------------
// Cronómetro (Impulsado por Heartbeats)
// ---------------------------------------------------------------------------

void CuestionarioService::_iniciarCronometro(int idCuestionario) {
    _idCuestionarioTimer = idCuestionario;
    _tiempoAcumuladoSeg  = 0;
}

void CuestionarioService::_pausarCronometro(int idCuestionario) {
    // Al pausar no hay que calcular nada.
    // El tiempo se congeló exacto en el último heartbeat.
}

int CuestionarioService::_tiempoTranscurridoSeg(int idCuestionario) {
    if (_idCuestionarioTimer != idCuestionario) return 0;
    return _tiempoAcumuladoSeg;
}

void CuestionarioService::procesarHeartbeatCronometro(int idCuestionario) {
    if (_idCuestionarioTimer == idCuestionario) {
        // Sumamos exactamente 2 segundos por cada heartbeat recibido
        _tiempoAcumuladoSeg += 2;
    }
}

// ---------------------------------------------------------------------------
// Crear
// ---------------------------------------------------------------------------

CuestionarioResult CuestionarioService::crear(const Cuestionario& c,
                                               const PreguntaCompleta* preguntas,
                                               int cant) {
    CuestionarioResult result;

    if (c.titulo.length() == 0) {
        result.mensaje = "El título no puede estar vacío.";
        return result;
    }

    if (cant < 1) {
        result.mensaje = "El cuestionario debe tener al menos una pregunta.";
        return result;
    }

    // Validar cada pregunta
    for (int i = 0; i < cant; i++) {
        if (preguntas[i].pregunta.pregunta.length() == 0) {
            result.mensaje = "La pregunta " + String(i + 1) + " no puede estar vacía.";
            return result;
        }
        if (preguntas[i].cantOpciones < 2 || preguntas[i].cantOpciones > 4) {
            result.mensaje = "La pregunta " + String(i + 1) + " debe tener entre 2 y 4 opciones.";
            return result;
        }

        int correctas = 0;
        for (int j = 0; j < preguntas[i].cantOpciones; j++) {
            if (preguntas[i].opciones[j].esCorrecta) correctas++;
        }
        if (correctas != 1) {
            result.mensaje = "La pregunta " + String(i + 1) + " debe tener exactamente una opción correcta.";
            return result;
        }
    }

    if (CuestionarioRepository::getInstance().existeTitulo(c.idUsuario, c.titulo)) {
        result.mensaje = "Ya existe un cuestionario con ese título.";
        return result;
    }

    // Crear cuestionario
    DbResult dbCues = CuestionarioRepository::getInstance().crear(c);
    if (!dbCues.ok) {
        result.mensaje = "Error al crear el cuestionario: " + dbCues.mensaje;
        return result;
    }

    int idCuestionario = dbCues.id;

    // Crear preguntas y opciones
    for (int i = 0; i < cant; i++) {
        Pregunta p = preguntas[i].pregunta;
        p.idCuestionario = idCuestionario;

        DbResult dbPreg = PreguntaRepository::getInstance().crear(p);
        if (!dbPreg.ok) {
            result.mensaje = "Error al crear la pregunta " + String(i + 1);
            return result;
        }

        int idPregunta       = dbPreg.id;
        int idOpcionCorrecta = 0;

        for (int j = 0; j < preguntas[i].cantOpciones; j++) {
            DbResult dbOpc = OpcionRepository::getInstance()
                              .crear(idPregunta, preguntas[i].opciones[j].opcion);
            if (!dbOpc.ok) {
                result.mensaje = "Error al crear la opción " + String(j + 1) +
                                 " de la pregunta " + String(i + 1);
                return result;
            }

            if (preguntas[i].opciones[j].esCorrecta) {
                idOpcionCorrecta = dbOpc.id;
            }
        }

        PreguntaRepository::getInstance().asignarOpcionCorrecta(idPregunta, idOpcionCorrecta);
    }

    result.ok = true;
    result.id = idCuestionario;
    return result;
}

// ---------------------------------------------------------------------------
// Eliminar
// ---------------------------------------------------------------------------

CuestionarioResult CuestionarioService::eliminar(int idCuestionario, int idUsuario) {
    CuestionarioResult result;

    Cuestionario c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);

    if (c.idCuestionario == 0) {
        result.mensaje = "Cuestionario no encontrado.";
        return result;
    }

    if (c.idUsuario != idUsuario) {
        result.mensaje = "No tenés permisos para eliminar este cuestionario.";
        return result;
    }

    if (c.estado == "en_progreso" || c.estado == "pausado") {
        result.mensaje = "No se puede eliminar un cuestionario en progreso o pausado.";
        return result;
    }

    // Eliminar opciones y preguntas antes del cuestionario (foreign keys)
    Pregunta preguntas[20];
    int cant = PreguntaRepository::getInstance()
                .listarPorCuestionario(idCuestionario, preguntas, 20);

    for (int i = 0; i < cant; i++) {
        OpcionRepository::getInstance().eliminarPorPregunta(preguntas[i].idPregunta);
        PreguntaRepository::getInstance().eliminar(preguntas[i].idPregunta);
    }

    DbResult db = CuestionarioRepository::getInstance().eliminar(idCuestionario);
    result.ok = db.ok;
    if (!result.ok) result.mensaje = db.mensaje;
    return result;
}

CuestionarioResult CuestionarioService::editar(const Cuestionario& c, const PreguntaCompleta* preguntas, int cant) {
    CuestionarioResult result;
    Cuestionario actual = CuestionarioRepository::getInstance().buscarPorId(c.idCuestionario);

    if (actual.idCuestionario == 0) { result.mensaje = "No encontrado."; return result; }
    if (actual.estado != "pendiente") { result.mensaje = "Solo se editan pendientes."; return result; }

    // 1. Actualizar datos base (título/puntaje)
    DbResult db = CuestionarioRepository::getInstance().actualizar(c);
    if (!db.ok) { result.mensaje = db.mensaje; return result; }

    // 2. Borrar preguntas y opciones viejas
    Pregunta pregsViejas[20];
    int cantViejas = PreguntaRepository::getInstance().listarPorCuestionario(c.idCuestionario, pregsViejas, 20);
    for (int i = 0; i < cantViejas; i++) {
        OpcionRepository::getInstance().eliminarPorPregunta(pregsViejas[i].idPregunta);
        PreguntaRepository::getInstance().eliminar(pregsViejas[i].idPregunta);
    }

    // 3. Crear preguntas y opciones nuevas (reutilizando tu lógica de crear)
    for (int i = 0; i < cant; i++) {
        Pregunta p = preguntas[i].pregunta;
        p.idCuestionario = c.idCuestionario;
        DbResult dbPreg = PreguntaRepository::getInstance().crear(p);
        
        int idPregunta = dbPreg.id;
        int idOpcionCorrecta = 0;

        for (int j = 0; j < preguntas[i].cantOpciones; j++) {
            DbResult dbOpc = OpcionRepository::getInstance().crear(idPregunta, preguntas[i].opciones[j].opcion);
            if (preguntas[i].opciones[j].esCorrecta) idOpcionCorrecta = dbOpc.id;
        }
        PreguntaRepository::getInstance().asignarOpcionCorrecta(idPregunta, idOpcionCorrecta);
    }

    result.ok = true;
    return result;
}

// ---------------------------------------------------------------------------
// Iniciar
// ---------------------------------------------------------------------------

CuestionarioResult CuestionarioService::iniciar(int idCuestionario, int idUsuario) {
    CuestionarioResult result;

    if (!_esDuenio(idCuestionario, idUsuario)) {
        result.mensaje = "Cuestionario no encontrado o sin permisos.";
        return result;
    }

    Cuestionario c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);

    if (c.estado != "pendiente") {
        result.mensaje = "Solo se puede iniciar un cuestionario pendiente.";
        return result;
    }

    if (CuestionarioRepository::getInstance().hayUnoEnProgreso()) {
        result.mensaje = "Ya hay un cuestionario en progreso. Pausalo o finalizalo primero.";
        return result;
    }

    DbResult db = CuestionarioRepository::getInstance()
                  .cambiarEstado(idCuestionario, "en_progreso");
    result.ok = db.ok;
    if (!result.ok) { 
        result.mensaje = db.mensaje; 
        return result; 
    }
    _iniciarCronometro(idCuestionario); 
    return result;
}

// ---------------------------------------------------------------------------
// Pausar
// ---------------------------------------------------------------------------

CuestionarioResult CuestionarioService::pausar(int idCuestionario, int idUsuario) {
    CuestionarioResult result;

    if (!_esDuenio(idCuestionario, idUsuario)) {
        result.mensaje = "Cuestionario no encontrado o sin permisos.";
        return result;
    }

    Cuestionario c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);

    if (c.estado != "en_progreso") {
        result.mensaje = "Solo se puede pausar un cuestionario en progreso.";
        return result;
    }

    DbResult db = CuestionarioRepository::getInstance()
                  .cambiarEstado(idCuestionario, "pausado");
    result.ok = db.ok;
    if (!result.ok) { 
        result.mensaje = db.mensaje; 
        return result; 
    }

    _pausarCronometro(idCuestionario); 
    return result;
}

// ---------------------------------------------------------------------------
// Reanudar
// ---------------------------------------------------------------------------

CuestionarioResult CuestionarioService::reanudar(int idCuestionario, int idUsuario) {
    CuestionarioResult result;

    if (!_esDuenio(idCuestionario, idUsuario)) {
        result.mensaje = "Cuestionario no encontrado o sin permisos.";
        return result;
    }

    Cuestionario c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);

    if (c.estado != "pausado") {
        result.mensaje = "Solo se puede reanudar un cuestionario pausado.";
        return result;
    }

    if (CuestionarioRepository::getInstance().hayUnoEnProgreso()) {
        result.mensaje = "Ya hay un cuestionario en progreso. Finalizalo primero.";
        return result;
    }

    // Limpiar respuestas anteriores para que el alumno empiece de nuevo
    PreguntaRepository::getInstance().limpiarRespuestas(idCuestionario);

    DbResult db = CuestionarioRepository::getInstance()
                  .cambiarEstado(idCuestionario, "en_progreso");
    result.ok = db.ok;
    if (!result.ok) { 
        result.mensaje = db.mensaje; 
        return result; 
    }

    _iniciarCronometro(idCuestionario); 
    return result;
}

// ---------------------------------------------------------------------------
// Finalizar
// ---------------------------------------------------------------------------

CuestionarioResult CuestionarioService::finalizar(int idCuestionario, int idUsuario) {
    CuestionarioResult result;

    if (!_esDuenio(idCuestionario, idUsuario)) {
        result.mensaje = "Cuestionario no encontrado o sin permisos.";
        return result;
    }

    Cuestionario c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);

    if (c.estado != "en_progreso" && c.estado != "pausado") {
        result.mensaje = "Solo se puede finalizar un cuestionario en progreso o pausado.";
        return result;
    }

    float puntaje = _calcularPuntaje(idCuestionario);
    
    // Extraer el tiempo del cronómetro antes de apagarlo ---
    int tiempoSegundos = _tiempoTranscurridoSeg(idCuestionario);

    String fecha = "2025-01-01T00:00:00";

    DbResult db = CuestionarioRepository::getInstance()
                  .guardarResultado(idCuestionario, puntaje, fecha, tiempoSegundos);
    result.ok = db.ok;
    if (!result.ok) result.mensaje = db.mensaje;
    
    // Apagamos el cronómetro
    _idCuestionarioTimer = 0; 
    
    return result;
}

// ---------------------------------------------------------------------------
// Obtener Completo (Para el Editor en React)
// ---------------------------------------------------------------------------
bool CuestionarioService::obtenerCompleto(int idCuestionario, Cuestionario& c, PreguntaCompleta* bufferPreguntas, int& cantPreguntas) {
    c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);
    if (c.idCuestionario == 0) return false;

    Pregunta* pregs = new Pregunta[20]; 
    cantPreguntas = PreguntaRepository::getInstance().listarPorCuestionario(idCuestionario, pregs, 20);

    for (int i = 0; i < cantPreguntas; i++) {
        bufferPreguntas[i].pregunta = pregs[i];
        
        Opcion opcs[4];
        int cantOp = OpcionRepository::getInstance().listarPorPregunta(pregs[i].idPregunta, opcs, 4);
        bufferPreguntas[i].cantOpciones = cantOp;
        
        for (int j = 0; j < cantOp; j++) {
            bufferPreguntas[i].opciones[j].opcion = opcs[j].opcion;
            bufferPreguntas[i].opciones[j].esCorrecta = (opcs[j].idOpcion == pregs[i].idOpcionCorrecta);
        }
    }
    
    delete[] pregs; 
    return true;
}

// ---------------------------------------------------------------------------
// Listados
// ---------------------------------------------------------------------------

int CuestionarioService::listarProfesor(int idUsuario,
                                         CuestionarioResumenProfesor* buffer,
                                         int maxSize) {
    return CuestionarioRepository::getInstance()
           .listarResumenProfesor(idUsuario, buffer, maxSize);
}

int CuestionarioService::listarTutor(CuestionarioResumenTutor* buffer, int maxSize) {
    return CuestionarioRepository::getInstance().listarResumenTutor(buffer, maxSize);
}

int CuestionarioService::obtenerRevision(int idCuestionario,
                                          PreguntaRevision* buffer, int maxSize) {
    Cuestionario c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);
    if (c.idCuestionario == 0) return -1;

    return PreguntaRepository::getInstance()
           .listarRevision(idCuestionario, buffer, maxSize);
}
