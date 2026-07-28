#include "ui/screens.h"
#include <Arduino.h>
#include "config/ConfigManager.h"

lv_obj_t * ta_clave1;
lv_obj_t * ta_clave2;
lv_obj_t * kb;
lv_obj_t * btn_guardar;
lv_obj_t * cont_form; // Contenedor con título, campos y botón (todo menos el teclado)

// --- Animación: desplaza el formulario verticalmente ---
static void anim_y_form_cb(void * obj, int32_t v) {
    lv_obj_set_y((lv_obj_t *)obj, v);
}

static void desplazar_formulario(lv_coord_t y_destino) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, cont_form);
    lv_anim_set_exec_cb(&a, anim_y_form_cb);
    lv_anim_set_values(&a, lv_obj_get_y(cont_form), y_destino);
    lv_anim_set_time(&a, 200);
    lv_anim_start(&a);
}

// --- Evento: Controla el teclado y quita el foco ---
static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = (lv_obj_t *)lv_event_get_target(e);
    
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        // Siempre arranca en minúsculas, sin importar en qué modo haya
        // quedado la vez anterior (por ej. si se tocó el Shift).
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        // Traemos el teclado al frente cada vez que se muestra. Como se crea
        // antes que los campos y el botón, sin esto quedaba dibujado DETRÁS
        // de ellos y se veía "solapado" en vez de taparlos prolijamente.
        lv_obj_move_foreground(kb);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);

        // Si el campo enfocado queda tapado por el teclado, corremos el
        // formulario hacia arriba lo justo y necesario para que se vea.
        lv_obj_update_layout(kb);
        lv_coord_t alto_teclado  = lv_obj_get_height(kb);
        lv_coord_t limite_visible = lv_display_get_vertical_resolution(NULL) - alto_teclado;

        // Coordenadas del campo relativas a cont_form (no cambian aunque
        // cont_form ya esté desplazado), así que sirven como referencia fija.
        lv_coord_t y2_campo = lv_obj_get_y(ta) + lv_obj_get_height(ta);

        lv_coord_t y_destino = 0;
        if(y2_campo > limite_visible) {
            y_destino = -(y2_campo - limite_visible + 10); // +10px de margen
        }
        desplazar_formulario(y_destino);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        desplazar_formulario(0); // Volvemos el formulario a su posición original
    }
    else if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        // Si el usuario toca el 'check' del teclado, lo cerramos
        lv_obj_remove_state(ta, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, ta); 
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        desplazar_formulario(0);
    }
}

// --- Evento: Guardar PALABRAS CLAVE via ConfigManager y reiniciar ---
static void btn_guardar_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        String clave1 = lv_textarea_get_text(ta_clave1); // Clave 1 -> rol PROFESOR
        String clave2 = lv_textarea_get_text(ta_clave2); // Clave 2 -> rol TUTOR

        if(clave1.length() == 0 || clave2.length() == 0) {
            Serial.println("[Setup] Error: Las claves no pueden estar vacias.");
            return; 
        }

        Serial.println("[Setup] Guardando palabras clave via ConfigManager...");

        // Usamos ConfigManager para que el namespace/keys reales (levi_cfg,
        // clave_prof, clave_tutor) queden en un solo lugar y no se desincronicen
        // con lo que lee clavesConfiguradas().
        bool ok1 = ConfigManager::getInstance().guardarClaveProfesor(clave1);
        bool ok2 = ConfigManager::getInstance().guardarClaveTutor(clave2);

        if(!ok1 || !ok2) {
            Serial.println("[Setup] Error al guardar una o ambas claves.");
            return;
        }

        Serial.println("[Setup] Guardado exitoso. Reiniciando el sistema...");
        delay(500);
        ESP.restart(); 
    }
}

// --- Evento: Muestra u oculta el texto de la clave asociada ---
static void toggle_pw_visibility_cb(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * ta  = (lv_obj_t *)lv_event_get_user_data(e);
 
    bool ahora_oculta = !lv_textarea_get_password_mode(ta);
    lv_textarea_set_password_mode(ta, ahora_oculta);
 
    // El ícono refleja el estado ACTUAL del campo: ojo abierto = se ve el
    // texto, ojo tachado = está oculto.
    lv_obj_t * icono = lv_obj_get_child(btn, 0);
    lv_label_set_text(icono, ahora_oculta ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
}
 
// --- Helper: crea el botón-ojo superpuesto al borde derecho de un textarea ---
static void agregar_boton_ojo(lv_obj_t * parent, lv_obj_t * ta_asociada) {
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_remove_style_all(btn); // sin fondo ni borde propios, solo el ícono
    lv_obj_set_size(btn, 30, 30);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
 
    lv_obj_t * icono = lv_label_create(btn);
    lv_obj_set_style_text_color(icono, lv_color_hex(0x555555), 0);
    lv_label_set_text(icono, LV_SYMBOL_EYE_CLOSE); // arranca oculta -> ojo tachado
    lv_obj_center(icono);
 
    // Va pegado al borde derecho del textarea correspondiente
    lv_obj_align_to(btn, ta_asociada, LV_ALIGN_RIGHT_MID, -6, 0);
 
    lv_obj_add_event_cb(btn, toggle_pw_visibility_cb, LV_EVENT_CLICKED, ta_asociada);
}
 
// --- Constructor de la pantalla de configuración ---
void ui_screen_setup_init(void) {
    lv_obj_t * pantalla_actual = lv_screen_active();
    lv_obj_set_style_bg_color(pantalla_actual, lv_color_hex(0x2C3E50), 0); 

    // 1. El Teclado Virtual (Oculto, cuelga del screen -no del formulario-
    //    para no moverse cuando desplazamos cont_form)
    kb = lv_keyboard_create(pantalla_actual);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    // 2. Contenedor invisible que agrupa título, campos y botón, así se
    // pueden desplazar juntos como un solo bloque sin tocar el teclado.
    cont_form = lv_obj_create(pantalla_actual);
    lv_obj_remove_style_all(cont_form); // sin fondo ni borde: totalmente transparente
    lv_obj_set_size(cont_form, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(cont_form, 0, 0);
    lv_obj_remove_flag(cont_form, LV_OBJ_FLAG_SCROLLABLE); // lo movemos a mano, no por scroll

    // 3. Título
    lv_obj_t * titulo = lv_label_create(cont_form);
    lv_label_set_text(titulo, "L.E.V.I. - Configurar Claves");
    lv_obj_set_style_text_color(titulo, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(titulo, &lv_font_montserrat_18, 0);
    lv_obj_align(titulo, LV_ALIGN_TOP_MID, 0, 10);

    // 4. Etiqueta + Input: Clave Profesor (centrados horizontalmente)
    lv_obj_t * lbl_clave1_titulo = lv_label_create(cont_form);
    lv_label_set_text(lbl_clave1_titulo, "Clave Profesor:");
    lv_obj_set_style_text_color(lbl_clave1_titulo, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_clave1_titulo, LV_ALIGN_TOP_MID, 0, 40);

    ta_clave1 = lv_textarea_create(cont_form);
    lv_textarea_set_one_line(ta_clave1, true);
    lv_textarea_set_password_mode(ta_clave1, true);
    lv_textarea_set_placeholder_text(ta_clave1, "Clave Profesor");
    lv_obj_set_width(ta_clave1, 220);
    lv_obj_align(ta_clave1, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_event_cb(ta_clave1, ta_event_cb, LV_EVENT_ALL, NULL);
    agregar_boton_ojo(cont_form, ta_clave1);

    // 5. Etiqueta + Input: Clave Tutor (centrados, debajo del anterior)
    lv_obj_t * lbl_clave2_titulo = lv_label_create(cont_form);
    lv_label_set_text(lbl_clave2_titulo, "Clave Tutor:");
    lv_obj_set_style_text_color(lbl_clave2_titulo, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_clave2_titulo, LV_ALIGN_TOP_MID, 0, 110);

    ta_clave2 = lv_textarea_create(cont_form);
    lv_textarea_set_one_line(ta_clave2, true);
    lv_textarea_set_password_mode(ta_clave2, true);
    lv_textarea_set_placeholder_text(ta_clave2, "Clave Tutor");
    lv_obj_set_width(ta_clave2, 220);
    lv_obj_align(ta_clave2, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_add_event_cb(ta_clave2, ta_event_cb, LV_EVENT_ALL, NULL);
    agregar_boton_ojo(cont_form, ta_clave2);

    // 6. Botón Guardar: rectangular, centrado, debajo de los dos campos
    // (estilo botón de "Iniciar sesión" de cualquier app de credenciales)
    btn_guardar = lv_button_create(cont_form);
    lv_obj_set_size(btn_guardar, 200, 45);
    lv_obj_align(btn_guardar, LV_ALIGN_TOP_MID, 0, 185);
    lv_obj_add_event_cb(btn_guardar, btn_guardar_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * lbl_btn = lv_label_create(btn_guardar);
    lv_label_set_text(lbl_btn, "Guardar");
    lv_obj_center(lbl_btn);
}