#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "config.h"
#include "camera_pins.h"

static httpd_handle_t pageServer = nullptr, streamServer = nullptr;
static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="nl"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WildCam cameratest</title><style>
body{font:18px system-ui;max-width:960px;margin:24px auto;padding:0 16px;background:#152025;color:#eef4f1}
img{display:block;width:100%;min-height:180px;background:#000;margin:20px 0}button,a{font:inherit;margin-right:12px;color:inherit}button{background:#304b41;padding:10px;border:1px solid #789;border-radius:6px}
</style><h1>WildCam cameratest</h1><p>Livebeeld van de ESP32-CAM. Gebruik voor deze test één kijker tegelijk.</p>
<button onclick="start()">Start livebeeld</button><button onclick="stop()">Stop</button>
<a href="/snapshot.jpg" target="_blank" onclick="stop()">Open foto</a><img id="cam" alt="Camerabeeld">
<p id="status">Livebeeld starten...</p><script>
const cam=document.getElementById('cam'),status=document.getElementById('status');
function start(){cam.src='http://'+location.hostname+':81/stream?t='+Date.now();status.textContent='Livebeeld aangevraagd (800 × 600 met PSRAM).';}
function stop(){cam.removeAttribute('src');status.textContent='Livebeeld gestopt.';}
cam.onerror=()=>status.textContent='Geen beeld ontvangen. Controleer wifi en probeer Start livebeeld.';
start();</script></html>)HTML";

static esp_err_t indexHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t snapshotHandler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera geeft geen frame");
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  esp_err_t result = httpd_resp_send(req, reinterpret_cast<const char *>(fb->buf), fb->len);
  esp_camera_fb_return(fb);
  return result;
}

static esp_err_t streamHandler(httpd_req_t *req) {
  esp_err_t result = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=wildcamframe");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  while (result == ESP_OK) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) { Serial.println("Stream: geen cameraframe"); return ESP_FAIL; }
    char header[128];
    int length = snprintf(header, sizeof(header), "\r\n--wildcamframe\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", unsigned(fb->len));
    result = httpd_resp_send_chunk(req, header, length);
    if (result == ESP_OK) result = httpd_resp_send_chunk(req, reinterpret_cast<const char *>(fb->buf), fb->len);
    esp_camera_fb_return(fb);
    delay(30);
  }
  return result;
}

static void check(esp_err_t result, const char *what) {
  if (result == ESP_OK) return;
  Serial.printf("FOUT %s: %s (0x%x). Controleer voeding/camerakabel en reset.\n", what, esp_err_to_name(result), result);
  while (true) delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  bool psram = psramFound();
  Serial.printf("\nWildCam cameratest; PSRAM: %s (%u bytes)\n", psram ? "ja" : "nee", ESP.getPsramSize());
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0; c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = CAM_D0; c.pin_d1 = CAM_D1; c.pin_d2 = CAM_D2; c.pin_d3 = CAM_D3;
  c.pin_d4 = CAM_D4; c.pin_d5 = CAM_D5; c.pin_d6 = CAM_D6; c.pin_d7 = CAM_D7;
  c.pin_xclk = CAM_XCLK; c.pin_pclk = CAM_PCLK; c.pin_vsync = CAM_VSYNC; c.pin_href = CAM_HREF;
  c.pin_sccb_sda = CAM_SDA; c.pin_sccb_scl = CAM_SCL;
  c.pin_pwdn = CAM_PWDN; c.pin_reset = CAM_RESET;
  c.xclk_freq_hz = 20000000; c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = psram ? FRAMESIZE_SVGA : FRAMESIZE_QVGA;
  c.jpeg_quality = 12; c.fb_count = psram ? 2 : 1;
  c.fb_location = psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  c.grab_mode = psram ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  check(esp_camera_init(&c), "camera initialiseren");
  Serial.printf("Camera gestart, sensor PID: 0x%x\n", esp_camera_sensor_get()->id.PID);
  bool connected = false;
  if (strlen(WIFI_SSID)) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(CAMERA_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Verbinden met wifi (maximaal 20 seconden)...");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) delay(250);
    connected = WiFi.status() == WL_CONNECTED;
  }
  if (connected) {
    WiFi.setAutoReconnect(true);
    Serial.printf("Open http://%s/\n", WiFi.localIP().toString().c_str());
    if (MDNS.begin(CAMERA_HOSTNAME)) MDNS.addService("http", "tcp", 80);
  } else {
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 2)) {
      Serial.println("FOUT: wifi-accesspoint starten mislukt");
      while (true) delay(1000);
    }
    Serial.printf("Verbind met wifi %s; open http://%s/\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  }
  WiFi.setSleep(false);
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.stack_size = 8192;
  config.send_wait_timeout = 5;
  check(httpd_start(&pageServer, &config), "webserver");
  httpd_uri_t index = {}; index.uri = "/"; index.method = HTTP_GET; index.handler = indexHandler;
  httpd_uri_t snapshot = {}; snapshot.uri = "/snapshot.jpg"; snapshot.method = HTTP_GET; snapshot.handler = snapshotHandler;
  check(httpd_register_uri_handler(pageServer, &index), "startpagina");
  check(httpd_register_uri_handler(pageServer, &snapshot), "foto endpoint");
  config.server_port = 81; config.ctrl_port += 1;
  check(httpd_start(&streamServer, &config), "streamserver");
  httpd_uri_t stream = {}; stream.uri = "/stream"; stream.method = HTTP_GET; stream.handler = streamHandler;
  check(httpd_register_uri_handler(streamServer, &stream), "stream endpoint");
  Serial.println("Gereed: webpagina op poort 80, MJPEG op poort 81.");
}

void loop() { delay(1000); }
