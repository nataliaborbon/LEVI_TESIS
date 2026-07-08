#include "screens.h"
#include <string.h>

// Variables globales para poder modificarlas luego desde el backend
lv_obj_t * tv_main;
lv_obj_t * lbl_alumno_main;
lv_obj_t * lbl_usuario_main;
lv_obj_t * lbl_camara_main;
lv_obj_t * lbl_disp_main;
lv_obj_t * lbl_estado_examen_main;
lv_obj_t * lbl_progreso_main;

// --- Función para actualizar el Usuario desde el backend ---
void ui_update_usuario(const char * nombre) {
  if(lbl_usuario_main) {
    if (strlen(nombre) == 0) {
      lv_label_set_text(lbl_usuario_main, "Sin conexion");
    } else {
      lv_label_set_text(lbl_usuario_main, nombre);
    }
  }
}

void ui_update_dispositivos(int cantidad) {
  if(lbl_disp_main) {
    lv_label_set_text_fmt(lbl_disp_main, "%d", cantidad);
  }
}

void ui_screen_main_init() {
  tv_main = lv_tileview_create(lv_screen_active());
  lv_obj_set_scrollbar_mode(tv_main, LV_SCROLLBAR_MODE_OFF); 

  // ==========================================
  // PANTALLA 1: BIENVENIDA (Columna 0, Fila 0)
  // ==========================================
  lv_obj_t * tile1 = lv_tileview_add_tile(tv_main, 0, 0, LV_DIR_HOR);
  lv_obj_set_style_bg_color(tile1, lv_color_hex(0xF0F0F0), 0);

  lv_obj_t * etiqueta_bienvenida = lv_label_create(tile1);
  lv_label_set_text(etiqueta_bienvenida, "Bienvenido\nIngresa a \"LEVI\"");
  lv_obj_set_style_text_align(etiqueta_bienvenida, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(etiqueta_bienvenida, &lv_font_montserrat_18, 0);
  lv_obj_align(etiqueta_bienvenida, LV_ALIGN_TOP_MID, 0, 15);

  lv_color_t color_oscuro = lv_color_hex(0x000000);
  lv_color_t color_claro = lv_color_hex(0xFFFFFF);

  lv_obj_t * qr = lv_qrcode_create(tile1);
  lv_qrcode_set_size(qr, 130);
  lv_qrcode_set_dark_color(qr, color_oscuro);
  lv_qrcode_set_light_color(qr, color_claro);
  lv_qrcode_update(qr, "levi.local", strlen("levi.local"));
  lv_obj_set_style_border_color(qr, color_oscuro, 0);
  lv_obj_set_style_border_width(qr, 3, 0);
  lv_obj_align_to(qr, etiqueta_bienvenida, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);

  // ==========================================
  // PANTALLA 2: ESTADO SISTEMA (Columna 1, Fila 0)
  // ==========================================
  lv_obj_t * tile2 = lv_tileview_add_tile(tv_main, 1, 0, LV_DIR_HOR);
  lv_obj_set_style_bg_color(tile2, lv_color_hex(0xF0F0F0), 0);

  lv_obj_t * titulo_estado = lv_label_create(tile2);
  lv_label_set_text(titulo_estado, "Estado");
  lv_obj_set_style_text_font(titulo_estado, &lv_font_montserrat_20, 0);
  lv_obj_align(titulo_estado, LV_ALIGN_TOP_MID, 0, 15);

  const lv_font_t * fuente_datos = &lv_font_montserrat_16;

  // --- ALUMNO ---
  lv_obj_t * lbl_alumno_titulo = lv_label_create(tile2);
  lv_label_set_text(lbl_alumno_titulo, "Alumno: ");
  lv_obj_set_style_text_font(lbl_alumno_titulo, fuente_datos, 0);
  lv_obj_align(lbl_alumno_titulo, LV_ALIGN_TOP_LEFT, 20, 60);

  lbl_alumno_main = lv_label_create(tile2);
  lv_label_set_text(lbl_alumno_main, "No registrado");
  lv_obj_set_style_text_font(lbl_alumno_main, fuente_datos, 0);
  lv_obj_align_to(lbl_alumno_main, lbl_alumno_titulo, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // --- USUARIO ---
  lv_obj_t * lbl_usuario_titulo = lv_label_create(tile2);
  lv_label_set_text(lbl_usuario_titulo, "Usuario: ");
  lv_obj_set_style_text_font(lbl_usuario_titulo, fuente_datos, 0);
  lv_obj_align(lbl_usuario_titulo, LV_ALIGN_TOP_LEFT, 20, 100);

  lbl_usuario_main = lv_label_create(tile2);
  lv_label_set_text(lbl_usuario_main, "Sin conexion");
  lv_obj_set_style_text_font(lbl_usuario_main, fuente_datos, 0);
  lv_obj_align_to(lbl_usuario_main, lbl_usuario_titulo, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // --- CAMARA ---
  lv_obj_t * lbl_camara_titulo = lv_label_create(tile2);
  lv_label_set_text(lbl_camara_titulo, "Camara: ");
  lv_obj_set_style_text_font(lbl_camara_titulo, fuente_datos, 0);
  lv_obj_align(lbl_camara_titulo, LV_ALIGN_TOP_LEFT, 20, 140);

  lbl_camara_main = lv_label_create(tile2);
  lv_label_set_text(lbl_camara_main, "Cargando...");
  lv_obj_set_style_text_font(lbl_camara_main, fuente_datos, 0);
  lv_obj_align_to(lbl_camara_main, lbl_camara_titulo, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // --- DISPOSITIVOS ---
  lv_obj_t * lbl_disp_titulo = lv_label_create(tile2);
  lv_label_set_text(lbl_disp_titulo, "Dispositivos conectados: ");
  lv_obj_set_style_text_font(lbl_disp_titulo, fuente_datos, 0);
  lv_obj_align(lbl_disp_titulo, LV_ALIGN_TOP_LEFT, 20, 180);

  lbl_disp_main = lv_label_create(tile2);
  lv_label_set_text(lbl_disp_main, "0");
  lv_obj_set_style_text_font(lbl_disp_main, fuente_datos, 0);
  lv_obj_align_to(lbl_disp_main, lbl_disp_titulo, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // ==========================================
  // PANTALLA 3: EXAMEN (Columna 2, Fila 0)
  // ==========================================
  lv_obj_t * tile3 = lv_tileview_add_tile(tv_main, 2, 0, LV_DIR_HOR);
  lv_obj_set_style_bg_color(tile3, lv_color_hex(0xF0F0F0), 0);

  lv_obj_t * titulo_examen = lv_label_create(tile3);
  lv_label_set_text(titulo_examen, "Examen\n\"Cuestionario X\"");
  lv_obj_set_style_text_align(titulo_examen, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(titulo_examen, &lv_font_montserrat_18, 0);
  lv_obj_align(titulo_examen, LV_ALIGN_TOP_MID, 0, 15);

  // --- ESTADO EXAMEN ---
  lv_obj_t * lbl_estado_examen_titulo = lv_label_create(tile3);
  lv_label_set_text(lbl_estado_examen_titulo, "Estado: ");
  lv_obj_set_style_text_font(lbl_estado_examen_titulo, fuente_datos, 0);
  lv_obj_align(lbl_estado_examen_titulo, LV_ALIGN_TOP_LEFT, 20, 90);

  lbl_estado_examen_main = lv_label_create(tile3);
  lv_label_set_text(lbl_estado_examen_main, "Pendiente");
  lv_obj_set_style_text_font(lbl_estado_examen_main, fuente_datos, 0);
  lv_obj_align_to(lbl_estado_examen_main, lbl_estado_examen_titulo, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // --- PROGRESO ---
  lv_obj_t * lbl_progreso_titulo = lv_label_create(tile3);
  lv_label_set_text(lbl_progreso_titulo, "Progreso: ");
  lv_obj_set_style_text_font(lbl_progreso_titulo, fuente_datos, 0);
  lv_obj_align(lbl_progreso_titulo, LV_ALIGN_TOP_LEFT, 20, 150);

  lbl_progreso_main = lv_label_create(tile3);
  lv_label_set_text(lbl_progreso_main, "0/0");
  lv_obj_set_style_text_font(lbl_progreso_main, fuente_datos, 0);
  lv_obj_align_to(lbl_progreso_main, lbl_progreso_titulo, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
}