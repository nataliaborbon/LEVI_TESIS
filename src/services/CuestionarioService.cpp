#include "services/CuestionarioService.h"
#include "storage/database/repositories/CuestionarioRepository.h"
#include "storage/database/repositories/PreguntaOpcionRepository.h"
#include "storage/database/DatabaseManager.h"
#include "esp_task_wdt.h"
#include "config/Limites.h"

// ---------------------------------------------------------------------------
// Helpers privados
// ---------------------------------------------------------------------------

bool CuestionarioService::_esDuenio(int idCuestionario, int idUsuario) {
    Cuestionario c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);
    return c.idCuestionario != 0 && c.idUsuario == idUsuario;
}

float CuestionarioService::_calcularPuntaje(int idCuestionario) {
    Pregunta preguntas[MAX_PREGUNTAS_POR_CUESTIONARIO];
    int cant = PreguntaRepository::getInstance()
                .listarPorCuestionario(idCuestionario, preguntas, MAX_PREGUNTAS_POR_CUESTIONARIO);

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

}

int CuestionarioService::_tiempoTranscurridoSeg(int idCuestionario) {
    if (_idCuestionarioTimer != idCuestionario) return 0;
    return _tiempoAcumuladoSeg;
}

void CuestionarioService::procesarHeartbeatCronometro(int idCuestionario) {
    if (_idCuestionarioTimer == idCuestionario) {
        _tiempoAcumuladoSeg += 2;
    }
}

// ---------------------------------------------------------------------------
// Crear
// ---------------------------------------------------------------------------

CuestionarioResult CuestionarioService::crear(const Cuestionario& c,
                                              const PreguntaCompleta* preguntas,
                                              int cant)
{
    CuestionarioResult result;

    Serial.println("[CREAR] Inicio");

    if (c.titulo.length() == 0)
    {
        result.mensaje = "El título no puede estar vacío.";
        return result;
    }

    if (cant < 1)
    {
        result.mensaje = "El cuestionario debe tener al menos una pregunta.";
        return result;
    }

    for (int i = 0; i < cant; i++)
    {
        if (preguntas[i].pregunta.pregunta.length() == 0)
        {
            result.mensaje = "Pregunta vacía";
            return result;
        }

        if (preguntas[i].cantOpciones < 2 ||
            preguntas[i].cantOpciones > 4)
        {
            result.mensaje = "Cantidad de opciones inválida";
            return result;
        }

        int correctas = 0;
        for (int j = 0; j < preguntas[i].cantOpciones; j++)
            if (preguntas[i].opciones[j].esCorrecta)
                correctas++;

        if (correctas != 1)
        {
            result.mensaje = "La pregunta " + String(i + 1) + " debe tener una correcta.";
            return result;
        }
    }

    if (CuestionarioRepository::getInstance().existeTitulo(c.idUsuario, c.titulo))
    {
        result.mensaje = "Ya existe un cuestionario con ese título.";
        return result;
    }


    auto abortarYLimpiar = [](int idCuestionario, const String& motivo) -> CuestionarioResult {
        CuestionarioResult r;
        r.mensaje = motivo;

        Serial.printf("[CREAR] ABORTANDO: %s\n", motivo.c_str());
        Serial.printf("[CREAR] Limpiando cuestionario ID=%d (cascade)\n", idCuestionario);

        DatabaseManager::getInstance().rollback();

        DbResult dbDel = CuestionarioRepository::getInstance().eliminar(idCuestionario);
        if (!dbDel.ok)
        {
            Serial.printf("[CREAR] ATENCION: no se pudo limpiar cuestionario ID=%d: %s\n",
                          idCuestionario, dbDel.mensaje.c_str());
        }

        return r;
    };

    if (!DatabaseManager::getInstance().beginTransaction())
    {
        result.mensaje = "Error iniciando transacción (cuestionario).";
        return result;
    }

    Serial.println("[CREAR] Insertando cuestionario");
    DbResult dbCues = CuestionarioRepository::getInstance().crear(c);
    if (!dbCues.ok)
    {
        DatabaseManager::getInstance().rollback();
        result.mensaje = dbCues.mensaje;
        return result;
    }

    if (!DatabaseManager::getInstance().commit())
    {
        DatabaseManager::getInstance().rollback();
        result.mensaje = "Error en commit (cuestionario).";
        return result;
    }

    int idCuestionario = dbCues.id;
    Serial.printf("[CREAR] Cuestionario creado ID=%d\n", idCuestionario);

    for (int i = 0; i < cant; i++)
    {
        esp_task_wdt_reset();
        yield();

        Serial.printf("[CREAR] Pregunta %d/%d\n", i + 1, cant);

        if (!DatabaseManager::getInstance().beginTransaction())
        {
            return abortarYLimpiar(idCuestionario,
                "Error iniciando transacción de pregunta " + String(i + 1));
        }

        Pregunta p = preguntas[i].pregunta;
        p.idCuestionario = idCuestionario;

        DbResult dbPreg = PreguntaRepository::getInstance().crear(p);
        if (!dbPreg.ok)
        {
            return abortarYLimpiar(idCuestionario,
                "Error creando pregunta " + String(i + 1) + ": " + dbPreg.mensaje);
        }

        if (!DatabaseManager::getInstance().commit())
        {
            return abortarYLimpiar(idCuestionario,
                "Error en commit de pregunta " + String(i + 1));
        }

        int idPregunta = dbPreg.id;
        esp_task_wdt_reset();
        yield();

        if (!DatabaseManager::getInstance().beginTransaction())
        {
            return abortarYLimpiar(idCuestionario,
                "Error iniciando transacción de opciones en pregunta " + String(i + 1));
        }

        int idOpcionCorrecta = 0;

        for (int j = 0; j < preguntas[i].cantOpciones; j++)
        {
            esp_task_wdt_reset();
            yield();

            DbResult dbOpc = OpcionRepository::getInstance()
                .crear(idPregunta, preguntas[i].opciones[j].opcion);
            if (!dbOpc.ok)
            {
                return abortarYLimpiar(idCuestionario,
                    "Error creando opción en pregunta " + String(i + 1) + ": " + dbOpc.mensaje);
            }
            if (preguntas[i].opciones[j].esCorrecta)
                idOpcionCorrecta = dbOpc.id;
        }

        if (idOpcionCorrecta == 0)
        {
            return abortarYLimpiar(idCuestionario,
                "No se encontró opción correcta en pregunta " + String(i + 1));
        }

        esp_task_wdt_reset();

        DbResult dbCorrecta = PreguntaRepository::getInstance()
            .asignarOpcionCorrecta(idPregunta, idOpcionCorrecta);
        if (!dbCorrecta.ok)
        {
            return abortarYLimpiar(idCuestionario,
                "Error al asignar opción correcta en pregunta " + String(i + 1) + ": " + dbCorrecta.mensaje);
        }

        if (!DatabaseManager::getInstance().commit())
        {
            return abortarYLimpiar(idCuestionario,
                "Error en commit de opciones en pregunta " + String(i + 1));
        }

        Serial.println("[CREAR] Pregunta completa OK");
        esp_task_wdt_reset();
        vTaskDelay(1);
    }

    Serial.println("[CREAR] FIN OK");
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

    Pregunta preguntas[MAX_PREGUNTAS_POR_CUESTIONARIO];
    int cant = PreguntaRepository::getInstance()
                .listarPorCuestionario(idCuestionario, preguntas, MAX_PREGUNTAS_POR_CUESTIONARIO);

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

    DbResult db = CuestionarioRepository::getInstance().actualizar(c);
    if (!db.ok) { result.mensaje = db.mensaje; return result; }

    Pregunta pregsViejas[MAX_PREGUNTAS_POR_CUESTIONARIO];
    int cantViejas = PreguntaRepository::getInstance()
                     .listarPorCuestionario(c.idCuestionario, pregsViejas, MAX_PREGUNTAS_POR_CUESTIONARIO);

    int idsNuevasCreadas[MAX_PREGUNTAS_POR_CUESTIONARIO];
    int cantNuevasCreadas = 0;

    auto limpiarNuevasYAbortar = [&](const String& motivo) -> CuestionarioResult {
        CuestionarioResult r;
        r.mensaje = motivo;

        Serial.printf("[EDITAR] ABORTANDO: %s\n", motivo.c_str());
        DatabaseManager::getInstance().rollback(); // por si quedó algo abierto

        for (int k = 0; k < cantNuevasCreadas; k++) {
            Serial.printf("[EDITAR] Limpiando pregunta nueva ID=%d (cascade)\n",
                          idsNuevasCreadas[k]);
            PreguntaRepository::getInstance().eliminar(idsNuevasCreadas[k]);
        }

        Serial.println("[EDITAR] Preguntas viejas quedaron intactas.");
        return r;
    };

    for (int i = 0; i < cant; i++)
    {
        esp_task_wdt_reset();
        yield();

        Serial.printf("[EDITAR] Pregunta nueva %d/%d\n", i + 1, cant);

        if (!DatabaseManager::getInstance().beginTransaction())
            return limpiarNuevasYAbortar("Error iniciando transacción de pregunta " + String(i + 1));

        Pregunta p = preguntas[i].pregunta;
        p.idCuestionario = c.idCuestionario;

        DbResult dbPreg = PreguntaRepository::getInstance().crear(p);
        if (!dbPreg.ok)
        {
            DatabaseManager::getInstance().rollback();
            return limpiarNuevasYAbortar("Error creando pregunta " + String(i + 1) + ": " + dbPreg.mensaje);
        }

        if (!DatabaseManager::getInstance().commit())
            return limpiarNuevasYAbortar("Error en commit de pregunta " + String(i + 1));

        int idPregunta = dbPreg.id;
        if (cantNuevasCreadas < MAX_PREGUNTAS_POR_CUESTIONARIO) idsNuevasCreadas[cantNuevasCreadas++] = idPregunta;

        esp_task_wdt_reset();
        yield();

        if (!DatabaseManager::getInstance().beginTransaction())
            return limpiarNuevasYAbortar("Error iniciando transacción de opciones en pregunta " + String(i + 1));

        int idOpcionCorrecta = 0;

        for (int j = 0; j < preguntas[i].cantOpciones; j++)
        {
            esp_task_wdt_reset();
            yield();

            DbResult dbOpc = OpcionRepository::getInstance()
                .crear(idPregunta, preguntas[i].opciones[j].opcion);
            if (!dbOpc.ok)
            {
                DatabaseManager::getInstance().rollback();
                return limpiarNuevasYAbortar("Error creando opción en pregunta " + String(i + 1) + ": " + dbOpc.mensaje);
            }
            if (preguntas[i].opciones[j].esCorrecta)
                idOpcionCorrecta = dbOpc.id;
        }

        if (idOpcionCorrecta == 0)
        {
            DatabaseManager::getInstance().rollback();
            return limpiarNuevasYAbortar("No se encontró opción correcta en pregunta " + String(i + 1));
        }

        esp_task_wdt_reset();

        DbResult dbCorrecta = PreguntaRepository::getInstance()
            .asignarOpcionCorrecta(idPregunta, idOpcionCorrecta);
        if (!dbCorrecta.ok)
        {
            DatabaseManager::getInstance().rollback();
            return limpiarNuevasYAbortar("Error al asignar opción correcta en pregunta " + String(i + 1) + ": " + dbCorrecta.mensaje);
        }

        if (!DatabaseManager::getInstance().commit())
            return limpiarNuevasYAbortar("Error en commit de opciones en pregunta " + String(i + 1));

        Serial.println("[EDITAR] Pregunta nueva completa OK");
        esp_task_wdt_reset();
        vTaskDelay(1);
    }

    Serial.println("[EDITAR] Todo lo nuevo OK. Borrando preguntas viejas...");
    for (int i = 0; i < cantViejas; i++) {
        esp_task_wdt_reset();
        OpcionRepository::getInstance().eliminarPorPregunta(pregsViejas[i].idPregunta);
        PreguntaRepository::getInstance().eliminar(pregsViejas[i].idPregunta);
    }

    Serial.println("[EDITAR] FIN OK");
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
    
    int tiempoSegundos = _tiempoTranscurridoSeg(idCuestionario);

    String fecha = "2025-01-01T00:00:00";

    DbResult db = CuestionarioRepository::getInstance()
                  .guardarResultado(idCuestionario, puntaje, fecha, tiempoSegundos);
    result.ok = db.ok;
    if (!result.ok) result.mensaje = db.mensaje;
    
    _idCuestionarioTimer = 0; 
    
    return result;
}

// ---------------------------------------------------------------------------
// Obtener Completo
// ---------------------------------------------------------------------------
bool CuestionarioService::obtenerCompleto(int idCuestionario, Cuestionario& c, PreguntaCompleta* bufferPreguntas, int& cantPreguntas) {
    c = CuestionarioRepository::getInstance().buscarPorId(idCuestionario);
    if (c.idCuestionario == 0) return false;

    Pregunta* pregs = new Pregunta[MAX_PREGUNTAS_POR_CUESTIONARIO]; 
    cantPreguntas = PreguntaRepository::getInstance().listarPorCuestionario(idCuestionario, pregs, MAX_PREGUNTAS_POR_CUESTIONARIO);

    for (int i = 0; i < cantPreguntas; i++) {
        bufferPreguntas[i].pregunta = pregs[i];
        
        Opcion opcs[MAX_OPCIONES_POR_PREGUNTA];
        int cantOp = OpcionRepository::getInstance().listarPorPregunta(pregs[i].idPregunta, opcs, MAX_OPCIONES_POR_PREGUNTA);
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