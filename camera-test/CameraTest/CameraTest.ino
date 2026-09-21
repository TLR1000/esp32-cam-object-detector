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
<form action="/" method="get">
<button type="submit" name="live" value="1">Start livebeeld</button>
<button type="submit" name="live" value="0">Stop</button>
<a href="/snapshot.jpg">Open foto</a></form>
%CAMERA_VIEW%
<p>Geen bewegend beeld? Open de <a href="%STREAM_URL%">directe stream</a> in je gewone browser.</p>
<p>Firmware: cameratest 2. Bediening werkt zonder JavaScript.</p></html>)HTML";

static esp_err_t indexHandler(httpd_req_t *req) {
  char query[64] = {}, live[8] = {};
  bool playing = true;
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
      httpd_query_key_value(query, "live", live, sizeof(live)) == ESP_OK) {
    playing = strcmp(live, "0") != 0;
  }
  const IPAddress address = WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP();
  String streamUrl = "http://" + address.toString() + ":81/stream";
  String page = PAGE;
  String view = playing ? "<img alt=\"Live camerabeeld\" src=\"" + streamUrl + "\">" : "<p>Livebeeld gestopt.</p>";
  page.replace("%CAMERA_VIEW%", view);
  page.replace("%STREAM_URL%", streamUrl);
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, page.c_str(), page.length());
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
  Serial.println("Livebeeld: kijker verbonden");
  unsigned frames = 0;
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
    if (result == ESP_OK && ++frames == 1) Serial.println("Livebeeld: eerste JPEG verzonden");
    delay(30);
  }
  Serial.printf("Livebeeld: verbinding gesloten na %u frames\n", frames);
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
    WiFi.setAutoReconnect(true);
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
    WiFi.mode(strlen(WIFI_SSID) ? WIFI_AP_STA : WIFI_AP);
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

void loop() {
  static bool wasConnected = false;
  static unsigned long lastRetry = 0;
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wasConnected) {
    Serial.printf("Wifi verbonden. Open http://%s/ of http://%s.local/\n", WiFi.localIP().toString().c_str(), CAMERA_HOSTNAME);
    MDNS.end();
    if (MDNS.begin(CAMERA_HOSTNAME)) MDNS.addService("http", "tcp", 80);
  }
  if (!connected && strlen(WIFI_SSID) && millis() - lastRetry >= 30000) {
    lastRetry = millis();
    Serial.println("Wifi: opnieuw verbinden...");
    WiFi.reconnect();
  }
  wasConnected = connected;
  delay(1000);
}
