// Start de camera als wifi-accesspoint en toont het livebeeld op een webpagina, met flits aan/uit.
#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "camera_pins.h"

static const char *AP_SSID = "ESP32CAM-Test";
static const char *AP_PASSWORD = "camera-test-32";
static bool flashOn = false;

static void setFlash(bool on) {
  flashOn = on;
  digitalWrite(FLASH_PIN, on ? HIGH : LOW);
}

// Pagina op poort 80: toont de stream van poort 81 en de flitsknoppen.
static esp_err_t pageHandler(httpd_req_t *req) {
  char query[16] = {}, value[4] = {};
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
      httpd_query_key_value(query, "flash", value, sizeof(value)) == ESP_OK) {
    setFlash(value[0] == '1');
  }
  String page = "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
                "<title>ESP32-CAM</title><body style='font:18px sans-serif;text-align:center'>"
                "<h1>ESP32-CAM</h1><p>Flits: <b>";
  page += flashOn ? "aan" : "uit";
  page += "</b> &nbsp; <a href='/?flash=1'>aan</a> | <a href='/?flash=0'>uit</a></p>"
          "<img style='width:100%;max-width:800px' src='http://" + WiFi.softAPIP().toString() + ":81/stream'></body>";
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, page.c_str(), page.length());
}

// MJPEG-stream op poort 81 (eigen server, zodat de pagina bedienbaar blijft).
static esp_err_t streamHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
  esp_err_t result = ESP_OK;
  while (result == ESP_OK) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) return ESP_FAIL;
    char header[96];
    int length = snprintf(header, sizeof(header), "\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", unsigned(fb->len));
    result = httpd_resp_send_chunk(req, header, length);
    if (result == ESP_OK) result = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
  return result;
}

static void startServer(int port, const char *uri, esp_err_t (*handler)(httpd_req_t *)) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = port;
  config.ctrl_port += port;
  httpd_handle_t server = nullptr;
  httpd_uri_t route = {uri, HTTP_GET, handler, nullptr};
  if (httpd_start(&server, &config) != ESP_OK || httpd_register_uri_handler(server, &route) != ESP_OK) {
    Serial.printf("FOUT: webserver op poort %d starten mislukt\n", port);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_PIN, OUTPUT);
  setFlash(false);

  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0; c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = CAM_D0; c.pin_d1 = CAM_D1; c.pin_d2 = CAM_D2; c.pin_d3 = CAM_D3;
  c.pin_d4 = CAM_D4; c.pin_d5 = CAM_D5; c.pin_d6 = CAM_D6; c.pin_d7 = CAM_D7;
  c.pin_xclk = CAM_XCLK; c.pin_pclk = CAM_PCLK; c.pin_vsync = CAM_VSYNC; c.pin_href = CAM_HREF;
  c.pin_sccb_sda = CAM_SDA; c.pin_sccb_scl = CAM_SCL;
  c.pin_pwdn = CAM_PWDN; c.pin_reset = CAM_RESET;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  bool psram = psramFound();
  c.frame_size = psram ? FRAMESIZE_SVGA : FRAMESIZE_QVGA;
  c.jpeg_quality = 12;
  c.fb_count = psram ? 2 : 1;
  c.fb_location = psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  c.grab_mode = psram ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  if (esp_camera_init(&c) != ESP_OK) {
    Serial.println("FOUT: camera starten mislukt. Controleer voeding en camerakabel.");
    while (true) delay(1000);
  }

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  WiFi.setSleep(false);

  startServer(80, "/", pageHandler);
  startServer(81, "/stream", streamHandler);
  Serial.printf("Verbind met wifi %s en open http://%s/\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void loop() {}
