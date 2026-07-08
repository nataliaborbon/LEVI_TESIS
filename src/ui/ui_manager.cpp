#include "ui_manager.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

// --- NUEVO: Incluimos las pantallas que diseñaste ---
#include "screens.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Pines táctiles CYD
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

TFT_eSPI tft = TFT_eSPI();

// Buffer alineado en memoria para LVGL 9
__attribute__((aligned(16))) static uint8_t draw_buf[SCREEN_WIDTH * SCREEN_HEIGHT / 20 * 2];

// =========================================================
// DRIVER TÁCTIL POR SOFTWARE (BITBANG)
// =========================================================

void touch_init()
{
  pinMode(XPT2046_CS, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);
  pinMode(XPT2046_CLK, OUTPUT);
  pinMode(XPT2046_MOSI, OUTPUT);
  pinMode(XPT2046_MISO, INPUT);
  pinMode(XPT2046_IRQ, INPUT);
}

uint16_t touch_read_spi(uint8_t cmd)
{
  uint16_t data = 0;
  digitalWrite(XPT2046_CS, LOW);
  for (int i = 7; i >= 0; i--)
  {
    digitalWrite(XPT2046_CLK, LOW);
    digitalWrite(XPT2046_MOSI, (cmd >> i) & 1);
    digitalWrite(XPT2046_CLK, HIGH);
  }
  digitalWrite(XPT2046_CLK, LOW);
  digitalWrite(XPT2046_CLK, HIGH);
  for (int i = 11; i >= 0; i--)
  {
    digitalWrite(XPT2046_CLK, LOW);
    data |= (digitalRead(XPT2046_MISO) << i);
    digitalWrite(XPT2046_CLK, HIGH);
  }
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(XPT2046_CLK, LOW);
    digitalWrite(XPT2046_CLK, HIGH);
  }
  digitalWrite(XPT2046_CS, HIGH);
  return data;
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  if (digitalRead(XPT2046_IRQ) == LOW)
  {
    data->state = LV_INDEV_STATE_PRESSED;
    uint16_t raw_y = touch_read_spi(0x90);
    uint16_t raw_x = touch_read_spi(0xD0);

    // CORRECCIÓN: Intercambiamos raw_x y raw_y porque la pantalla está en Landscape.
    // Si al deslizar a la izquierda la pantalla va a la derecha (movimiento invertido),
    // solo tienes que dar vuelta el 200 y el 3800 así: map(raw_y, 3800, 200, 0, SCREEN_WIDTH)
    data->point.x = map(raw_y, 200, 3800, 0, SCREEN_WIDTH);
    data->point.y = map(raw_x, 200, 3800, 0, SCREEN_HEIGHT);
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// =========================================================
// DRIVER DE PANTALLA LVGL
// =========================================================

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

static uint32_t my_tick_get_cb(void)
{
  return millis();
}

// --- FUNCIONES PÚBLICAS ---

void ui_init(bool isFirstBoot)
{
  // 1. Inicializar Pantalla (Usa Hardware SPI)
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // 2. Inicializar Táctil (Usa Software SPI)
  touch_init();

  // 3. Inicializar LVGL
  lv_init();
  lv_tick_set_cb(my_tick_get_cb);

  lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  // 4. DECIDIR QUÉ INTERFAZ CARGAR
  if (isFirstBoot)
  {
    // Todavía no programamos el teclado de guardado, así que dejamos este texto provisorio
    lv_obj_t *pantalla_actual = lv_screen_active();
    lv_obj_set_style_bg_color(pantalla_actual, lv_color_hex(0xF0F0F0), 0);

    lv_obj_t *etiqueta_prueba = lv_label_create(pantalla_actual);
    lv_obj_set_style_text_color(etiqueta_prueba, lv_color_hex(0x000000), 0);
    lv_label_set_text(etiqueta_prueba, "L.E.V.I.\nMODO CONFIGURACION INICIAL");
    lv_obj_set_style_text_align(etiqueta_prueba, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(etiqueta_prueba, LV_ALIGN_CENTER, 0, 0);
  }
  else
  {
    // --- NUEVO: Llamamos a la función de tu archivo screen_main.cpp ---
    ui_screen_main_init();
  }
}

void ui_loop()
{
  static uint32_t t0;
  static uint32_t espera;
  const uint32_t t = millis();
  if (t - t0 > espera)
  {
    espera = lv_timer_handler();
    t0 = t;
  }
}