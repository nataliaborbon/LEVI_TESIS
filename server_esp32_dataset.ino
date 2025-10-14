#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// --- CONFIG WIFI ---
const char* ssid = "abc";
const char* password = "s13m12c23";

// --- SERVIDOR ---
const char* serverUrl = "http://10.154.102.195:5000/upload";

// --- CAMERA MODEL ---
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// --- FLAGS ---
bool photoRequested = false;

// --- INICIALIZA CÁMARA ---
void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE; // Escala de grises

  if(psramFound()){
    config.frame_size = FRAMESIZE_QQVGA; // 160x120
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.fb_count = 1;
  }

  if(esp_camera_init(&config) != ESP_OK){
    Serial.println("Error inicializando cámara");
    while(true);
  }
  Serial.println("Cámara lista en escala de grises");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Conectado! IP: " + WiFi.localIP().toString());

  startCamera();
  Serial.println("Envía 'c' por Serial para capturar foto y enviarla al servidor.");
}

void loop() {
  // Revisar entrada Serial
  if(Serial.available()){
    char ch = Serial.read();
    if(ch=='c'){
      photoRequested = true;
    }
  }

  if(photoRequested){
    photoRequested = false;
    camera_fb_t* fb = esp_camera_fb_get();
    if(!fb){
      Serial.println("Error al capturar imagen");
      return;
    }
    Serial.println("Capturando imagen...");

    // Crear buffer PGM
    String header = "P5\n" + String(fb->width) + " " + String(fb->height) + "\n255\n";
    int totalSize = header.length() + fb->len;
    uint8_t* buffer = new uint8_t[totalSize];
    memcpy(buffer, header.c_str(), header.length());
    memcpy(buffer + header.length(), fb->buf, fb->len);

    // Enviar al servidor
    if(WiFi.status()==WL_CONNECTED){
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/octet-stream");
      int code = http.POST(buffer, totalSize);
      if(code > 0){
        Serial.printf("Imagen enviada! HTTP code: %d\n", code);
      } else {
        Serial.printf("Error al enviar imagen: %d\n", code);
      }
      http.end();
    } else {
      Serial.println("WiFi desconectado");
    }

    delete[] buffer;
    esp_camera_fb_return(fb);
  }

  delay(100);
}


