#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// --- CONFIG WIFI ---
const char* ssid = "ABC";
const char* password = "S13M12C23";

// --- WEBSERVER ---
WebServer server(80);

// --- CAMERA MODEL ---
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// --- CALIBRACIÓN ---
int calibX = -1, calibY = -1;
bool calibrated = false;
bool streamActive = true;

// --- INICIALIZA CÁMARA ---
void startCamera(bool useJPEG=true) {
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
  config.pixel_format = useJPEG ? PIXFORMAT_JPEG : PIXFORMAT_GRAYSCALE;

  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if(esp_camera_init(&config) != ESP_OK){
    Serial.println("Error inicializando cámara");
    while(true);
  }

  Serial.println("Cámara lista");
}

// --- STREAM ---
void handle_stream() {
  if(!streamActive){
    server.send(200, "text/plain", "Stream desactivado tras calibracion");
    return;
  }

  WiFiClient client = server.client();
  String header = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(header);

  while(client.connected() && streamActive){
    camera_fb_t* fb = esp_camera_fb_get();
    if(!fb) break;

    client.printf("--frame\r\n");
    client.printf("Content-Type: image/jpeg\r\n");
    client.printf("Content-Length: %d\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.printf("\r\n");

    esp_camera_fb_return(fb);
    delay(50); // 20 fps aprox
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi. IP: " + WiFi.localIP().toString());

  startCamera(true); // JPEG al inicio

  server.on("/", handle_stream);
  server.begin();
  Serial.println("Servidor web listo! Stream activo.");
  Serial.println("Envía 'c' por Serial para calibrar y cambiar a detección");
}

// --- LOOP ---
void loop() {
  server.handleClient();

  if(Serial.available()){
    char ch = Serial.read();
    if(ch=='c' && !calibrated){
      Serial.println("Calibrando y cambiando a grayscale...");
      streamActive = false;
      delay(100);
      esp_camera_deinit();
      startCamera(false); // cambia a grayscale
      calibrated = true;
      Serial.println("Modo deteccion activado. Stream detenido.");
    }
  }

  if(calibrated){
    camera_fb_t* fb = esp_camera_fb_get();
    if(!fb) return;

    int width = fb->width;
    int height = fb->height;
    uint8_t* data = fb->buf;

    int blockSize = 8;
    int bestX=0, bestY=0;
    int minAvg = 255;

    for(int by=0; by<height; by+=blockSize){
      for(int bx=0; bx<width; bx+=blockSize){
        long sum=0; int count=0;
        for(int y=0; y<blockSize; y++){
          for(int x=0; x<blockSize; x++){
            int px = bx+x;
            int py = by+y;
            if(px<width && py<height){
              int i = py*width+px;
              sum += data[i];
              count++;
            }
          }
        }
        int avg = sum/count;
        if(avg<minAvg){
          minAvg=avg;
          bestX = bx+blockSize/2;
          bestY = by+blockSize/2;
        }
      }
    }

    esp_camera_fb_return(fb);

    if(calibX<0 || calibY<0){
      calibX = bestX;
      calibY = bestY;
      Serial.printf("Calibrado en (%d,%d)\n", calibX, calibY);
      return;
    }

    int dx = bestX - calibX;
    int dy = bestY - calibY;
    String dir="Centro";
    int thrX = width/12;
    int thrY = height/12;

    if(abs(dx)>thrX || abs(dy)>thrY){
      if(dy<-thrY && abs(dy)>abs(dx)) dir="Arriba";
      else if(dy>thrY && abs(dy)>abs(dx)) dir="Abajo";
      else if(dx<-thrX && abs(dx)>abs(dy)) dir="Izquierda";
      else if(dx>thrX && abs(dx)>abs(dy)) dir="Derecha";
      else if(dx<-thrX && dy<-thrY) dir="Arriba-Izq";
      else if(dx>thrX && dy<-thrY) dir="Arriba-Der";
      else if(dx<-thrX && dy>thrY) dir="Abajo-Izq";
      else if(dx>thrX && dy>thrY) dir="Abajo-Der";
    }

    Serial.printf("Pupila en (%d,%d) -> %s\n", bestX, bestY, dir.c_str());
    delay(200);
  }
}

