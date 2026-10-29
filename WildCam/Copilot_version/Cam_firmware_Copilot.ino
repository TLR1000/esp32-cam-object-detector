#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_camera.h>
#include <img_converters.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <time.h>
#include <sys/time.h>

#ifndef WILDCAM_WIFI_SSID
#define WILDCAM_WIFI_SSID ""
#endif

#ifndef WILDCAM_WIFI_PASSWORD
#define WILDCAM_WIFI_PASSWORD ""
#endif

static const char *FIRMWARE_VERSION = "1.0.0";
static const char *PREFERENCES_NAMESPACE = "wildcam";
static const uint32_t MAX_QUEUE_BYTES = 2U * 1024U * 1024U;
static const uint16_t DETECT_WIDTH = 320;
static const uint16_t DETECT_HEIGHT = 240;
static const uint8_t GRID_COLUMNS = 8;
static const uint8_t GRID_ROWS = 6;
static const uint8_t GRID_CELLS = GRID_COLUMNS * GRID_ROWS;
static const uint8_t DETECT_FPS = 5;
static const uint8_t EVENT_FPS = 3;
static const uint32_t EVENT_DURATION_MS = 3000;
static const uint32_t COOLDOWN_MS = 2000;
static const uint8_t EVENT_TRIGGER_FRAMES = 2;
static const framesize_t EVENT_FRAME_SIZE = FRAMESIZE_SVGA;

// AI-Thinker ESP32-CAM / OV2640 camera pin mapping.
static const int CAMERA_PWDN_GPIO = 32;
static const int CAMERA_RESET_GPIO = -1;
static const int CAMERA_XCLK_GPIO = 0;
static const int CAMERA_SIOD_GPIO = 26;
static const int CAMERA_SIOC_GPIO = 27;
static const int CAMERA_D7_GPIO = 35;
static const int CAMERA_D6_GPIO = 34;
static const int CAMERA_D5_GPIO = 39;
static const int CAMERA_D4_GPIO = 36;
static const int CAMERA_D3_GPIO = 21;
static const int CAMERA_D2_GPIO = 19;
static const int CAMERA_D1_GPIO = 18;
static const int CAMERA_D0_GPIO = 5;
static const int CAMERA_VSYNC_GPIO = 25;
static const int CAMERA_HREF_GPIO = 23;
static const int CAMERA_PCLK_GPIO = 22;

struct CameraSettings {
  String cameraId;
  String wifiSsid;
  String wifiPassword;
  String backendUrl;
  uint16_t roiX;
  uint16_t roiY;
  uint16_t roiW;
  uint16_t roiH;
  uint8_t jpegQuality;
  uint8_t motionThreshold;
  uint8_t globalChangeLimit;
  uint8_t detectFps;
  uint8_t triggerFrames;
  uint8_t eventSeconds;
  uint8_t eventFps;
  uint16_t cooldownMs;
};

struct QueuedFrame {
  uint8_t *jpeg;
  size_t length;
  uint64_t eventId;
  uint16_t frameIndex;
  uint64_t captureMillis;
  char capturedAt[32];
  int rssi;
  QueuedFrame *next;
};

struct QueueSnapshot {
  uint32_t frames;
  uint32_t bytes;
};

struct CounterSnapshot {
  uint32_t detections;
  uint32_t events;
  uint32_t framesCaptured;
  uint32_t framesUploaded;
  uint32_t droppedFrames;
  uint32_t uploadFailures;
  uint32_t cameraFailures;
  uint32_t lastTriggerMillis;
  int lastBackendStatus;
};

static CameraSettings settings;
static Preferences preferences;
static WebServer webServer(80);
static SemaphoreHandle_t cameraMutex;
static SemaphoreHandle_t queueMutex;
static SemaphoreHandle_t counterMutex;
static SemaphoreHandle_t eventMutex;
static QueuedFrame *queueHead = nullptr;
static QueuedFrame *queueTail = nullptr;
static QueueSnapshot queued = {0, 0};
static CounterSnapshot counters = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint32_t bootId = 0;
static uint64_t nextEventId = 1;
static uint8_t *rgbScratch = nullptr;
static bool previousGridValid = false;
static uint8_t previousGrid[GRID_CELLS] = {0};
static uint8_t triggerHistory[3] = {0, 0, 0};
static uint8_t triggerHistoryIndex = 0;
static uint32_t wifiRetryAt = 0;
static uint32_t wifiStartedAt = 0;
static uint32_t heartbeatAt = 0;
static bool mdnsStarted = false;
static bool setupApStarted = false;

static void incrementCounter(uint32_t CounterSnapshot::*member, uint32_t amount = 1) {
  xSemaphoreTake(counterMutex, portMAX_DELAY);
  counters.*member += amount;
  xSemaphoreGive(counterMutex);
}

static void setBackendStatus(int status) {
  xSemaphoreTake(counterMutex, portMAX_DELAY);
  counters.lastBackendStatus = status;
  xSemaphoreGive(counterMutex);
}

static uint64_t uptimeMillis() {
  return static_cast<uint64_t>(esp_timer_get_time()) / 1000;
}

static String uint64String(uint64_t value) {
  char text[21];
  snprintf(text, sizeof(text), "%llu", static_cast<unsigned long long>(value));
  return String(text);
}

static CounterSnapshot getCounters() {
  CounterSnapshot copy;
  xSemaphoreTake(counterMutex, portMAX_DELAY);
  copy = counters;
  xSemaphoreGive(counterMutex);
  return copy;
}

static QueueSnapshot getQueueSnapshot() {
  QueueSnapshot copy;
  xSemaphoreTake(queueMutex, portMAX_DELAY);
  copy = queued;
  xSemaphoreGive(queueMutex);
  return copy;
}

static bool validCameraId(const String &id) {
  if (!id.startsWith("wildcam") || id.length() <= 7) {
    return false;
  }
  if (id[7] < '1' || id[7] > '9') {
    return false;
  }
  for (size_t i = 7; i < id.length(); ++i) {
    if (id[i] < '0' || id[i] > '9') {
      return false;
    }
  }
  return true;
}

static bool parseBoundedInteger(const String &value, long minimum, long maximum, long &result) {
  if (value.isEmpty()) {
    return false;
  }
  char *end = nullptr;
  const long parsed = strtol(value.c_str(), &end, 10);
  if (end == value.c_str() || *end != '\0' || parsed < minimum || parsed > maximum) {
    return false;
  }
  result = parsed;
  return true;
}

static String jsonEscape(const String &value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char ch = value[i];
    if (ch == '"' || ch == '\\') {
      escaped += '\\';
    }
    if (static_cast<uint8_t>(ch) >= 0x20) {
      escaped += ch;
    }
  }
  return escaped;
}

static void loadSettings() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  settings.cameraId = preferences.getString("camera_id", "wildcam1");
  settings.wifiSsid = preferences.getString("wifi_ssid", WILDCAM_WIFI_SSID);
  settings.wifiPassword = preferences.getString("wifi_password", WILDCAM_WIFI_PASSWORD);
  settings.backendUrl = preferences.getString("backend_url", "http://shoebox:8088");
  settings.roiX = preferences.getUShort("roi_x", 0);
  settings.roiY = preferences.getUShort("roi_y", 0);
  settings.roiW = preferences.getUShort("roi_w", 1000);
  settings.roiH = preferences.getUShort("roi_h", 1000);
  settings.jpegQuality = preferences.getUChar("jpeg_quality", 12);
  settings.motionThreshold = preferences.getUChar("motion_threshold", 18);
  settings.globalChangeLimit = preferences.getUChar("global_limit", 65);
  settings.detectFps = preferences.getUChar("detect_fps", DETECT_FPS);
  settings.triggerFrames = preferences.getUChar("trigger_frames", EVENT_TRIGGER_FRAMES);
  settings.eventSeconds = preferences.getUChar("event_seconds", EVENT_DURATION_MS / 1000);
  settings.eventFps = preferences.getUChar("event_fps", EVENT_FPS);
  settings.cooldownMs = preferences.getUShort("cooldown_ms", COOLDOWN_MS);
  nextEventId = preferences.getULong64("next_event_id", 1);
  if (!validCameraId(settings.cameraId)) {
    settings.cameraId = "wildcam1";
  }
  if (settings.roiW == 0 || settings.roiH == 0 ||
      settings.roiX + settings.roiW > 1000 || settings.roiY + settings.roiH > 1000) {
    settings.roiX = 0;
    settings.roiY = 0;
    settings.roiW = 1000;
    settings.roiH = 1000;
  }
  if (settings.detectFps < 1 || settings.detectFps > 10) {
    settings.detectFps = DETECT_FPS;
  }
  if (settings.triggerFrames < 1 || settings.triggerFrames > 3) {
    settings.triggerFrames = EVENT_TRIGGER_FRAMES;
  }
  if (settings.eventSeconds < 1 || settings.eventSeconds > 60) {
    settings.eventSeconds = EVENT_DURATION_MS / 1000;
  }
  if (settings.eventFps < 1 || settings.eventFps > 10) {
    settings.eventFps = EVENT_FPS;
  }
  if (settings.cooldownMs < 500 || settings.cooldownMs > 60000) {
    settings.cooldownMs = COOLDOWN_MS;
  }
  if (settings.jpegQuality < 10 || settings.jpegQuality > 63) {
    settings.jpegQuality = 12;
  }
  if (settings.motionThreshold < 1 || settings.motionThreshold > 100) {
    settings.motionThreshold = 18;
  }
  if (settings.globalChangeLimit < 10 || settings.globalChangeLimit > 100) {
    settings.globalChangeLimit = 65;
  }
  if (!settings.backendUrl.startsWith("http://")) {
    settings.backendUrl = "http://shoebox:8088";
  }
}

static camera_config_t makeCameraConfig() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAMERA_D0_GPIO;
  config.pin_d1 = CAMERA_D1_GPIO;
  config.pin_d2 = CAMERA_D2_GPIO;
  config.pin_d3 = CAMERA_D3_GPIO;
  config.pin_d4 = CAMERA_D4_GPIO;
  config.pin_d5 = CAMERA_D5_GPIO;
  config.pin_d6 = CAMERA_D6_GPIO;
  config.pin_d7 = CAMERA_D7_GPIO;
  config.pin_xclk = CAMERA_XCLK_GPIO;
  config.pin_pclk = CAMERA_PCLK_GPIO;
  config.pin_vsync = CAMERA_VSYNC_GPIO;
  config.pin_href = CAMERA_HREF_GPIO;
  config.pin_sccb_sda = CAMERA_SIOD_GPIO;
  config.pin_sccb_scl = CAMERA_SIOC_GPIO;
  config.pin_pwdn = CAMERA_PWDN_GPIO;
  config.pin_reset = CAMERA_RESET_GPIO;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = settings.jpegQuality;
  config.fb_count = psramFound() ? 2 : 1;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  return config;
}

static bool initializeCamera() {
  const camera_config_t config = makeCameraConfig();
  const esp_err_t result = esp_camera_init(&config);
  if (result != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", result);
    return false;
  }
  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor == nullptr || sensor->set_framesize(sensor, FRAMESIZE_QVGA) != 0) {
    Serial.println("Camera sensor configuration failed");
    esp_camera_deinit();
    return false;
  }
  return true;
}

static bool setCameraFrameSize(framesize_t frameSize) {
  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor == nullptr || sensor->set_framesize(sensor, frameSize) != 0) {
    Serial.println("Could not change camera frame size");
    incrementCounter(&CounterSnapshot::cameraFailures);
    return false;
  }
  return true;
}

static bool capturedAtIso8601(char output[32]) {
  struct timeval timestamp;
  if (gettimeofday(&timestamp, nullptr) != 0 || timestamp.tv_sec < 1700000000) {
    output[0] = '\0';
    return false;
  }
  struct tm utcTime;
  if (gmtime_r(&timestamp.tv_sec, &utcTime) == nullptr) {
    output[0] = '\0';
    return false;
  }
  char date[24];
  strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%S", &utcTime);
  snprintf(output, 32, "%s.%03ldZ", date, static_cast<long>(timestamp.tv_usec / 1000));
  return true;
}

static void formatBootId(char output[9]) {
  snprintf(output, 9, "%08lx", static_cast<unsigned long>(bootId));
}

static bool enqueueFrame(const uint8_t *jpeg, size_t length, uint64_t eventId,
                         uint16_t frameIndex, uint64_t captureMillis) {
  if (jpeg == nullptr || length == 0 || length > MAX_QUEUE_BYTES) {
    incrementCounter(&CounterSnapshot::droppedFrames);
    return false;
  }

  QueuedFrame *entry = static_cast<QueuedFrame *>(ps_malloc(sizeof(QueuedFrame)));
  uint8_t *copy = static_cast<uint8_t *>(ps_malloc(length));
  if (entry == nullptr || copy == nullptr) {
    if (entry != nullptr) {
      free(entry);
    }
    if (copy != nullptr) {
      free(copy);
    }
    incrementCounter(&CounterSnapshot::droppedFrames);
    return false;
  }
  memcpy(copy, jpeg, length);
  entry->jpeg = copy;
  entry->length = length;
  entry->eventId = eventId;
  entry->frameIndex = frameIndex;
  entry->captureMillis = captureMillis;
  capturedAtIso8601(entry->capturedAt);
  entry->rssi = WiFi.isConnected() ? WiFi.RSSI() : 0;
  entry->next = nullptr;

  xSemaphoreTake(queueMutex, portMAX_DELAY);
  if (queued.bytes + length > MAX_QUEUE_BYTES) {
    xSemaphoreGive(queueMutex);
    free(copy);
    free(entry);
    incrementCounter(&CounterSnapshot::droppedFrames);
    return false;
  }
  if (queueTail == nullptr) {
    queueHead = entry;
    queueTail = entry;
  } else {
    queueTail->next = entry;
    queueTail = entry;
  }
  queued.frames++;
  queued.bytes += length;
  xSemaphoreGive(queueMutex);
  incrementCounter(&CounterSnapshot::framesCaptured);
  return true;
}

static void removeQueueHead(QueuedFrame *expectedHead) {
  xSemaphoreTake(queueMutex, portMAX_DELAY);
  if (queueHead == expectedHead) {
    queueHead = expectedHead->next;
    if (queueHead == nullptr) {
      queueTail = nullptr;
    }
    queued.frames--;
    queued.bytes -= expectedHead->length;
    free(expectedHead->jpeg);
    free(expectedHead);
  }
  xSemaphoreGive(queueMutex);
}

static String framesEndpoint() {
  String base = settings.backendUrl;
  while (base.endsWith("/")) {
    base.remove(base.length() - 1);
  }
  return base + "/api/v1/frames";
}

static bool uploadFrame(QueuedFrame *entry, int &httpStatus) {
  if (!WiFi.isConnected()) {
    httpStatus = 0;
    return false;
  }
  HTTPClient http;
  http.setConnectTimeout(4000);
  http.setTimeout(5000);
  if (!http.begin(framesEndpoint())) {
    httpStatus = 0;
    return false;
  }
  char bootIdText[9];
  formatBootId(bootIdText);
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("X-WildCam-ID", settings.cameraId);
  http.addHeader("X-Boot-ID", bootIdText);
  http.addHeader("X-Event-ID", uint64String(entry->eventId));
  http.addHeader("X-Frame-Index", String(entry->frameIndex));
  http.addHeader("X-Capture-Millis", uint64String(entry->captureMillis));
  if (entry->capturedAt[0] != '\0') {
    http.addHeader("X-Captured-At", entry->capturedAt);
  }
  http.addHeader("X-RSSI", String(entry->rssi));
  http.addHeader("X-Firmware-Version", FIRMWARE_VERSION);
  http.addHeader("X-Protocol-Version", "1");
  const int result = http.sendRequest("POST", entry->jpeg, entry->length);
  httpStatus = result;
  http.end();
  return result >= 200 && result < 300;
}

static bool postHeartbeat() {
  if (!WiFi.isConnected()) {
    return false;
  }
  const QueueSnapshot queue = getQueueSnapshot();
  const CounterSnapshot stats = getCounters();
  char bootIdText[9];
  formatBootId(bootIdText);
  String body = "{\"camera_id\":\"" + jsonEscape(settings.cameraId) +
                "\",\"boot_id\":\"" + bootIdText +
                "\",\"uptime_s\":" + uint64String(uptimeMillis() / 1000) +
                ",\"ip\":\"" + WiFi.localIP().toString() +
                "\",\"rssi\":" + String(WiFi.RSSI()) +
                ",\"queue_frames\":" + String(queue.frames) +
                ",\"queue_bytes\":" + String(queue.bytes) +
                ",\"dropped_frames\":" + String(stats.droppedFrames) +
                ",\"upload_failures\":" + String(stats.uploadFailures) +
                ",\"firmware_version\":\"" + FIRMWARE_VERSION + "\"}";
  String base = settings.backendUrl;
  while (base.endsWith("/")) {
    base.remove(base.length() - 1);
  }
  HTTPClient http;
  http.setConnectTimeout(4000);
  http.setTimeout(5000);
  if (!http.begin(base + "/api/v1/heartbeat")) {
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  const int result = http.POST(body);
  http.end();
  return result >= 200 && result < 300;
}

static bool enqueueCapturedFrame(uint64_t eventId, uint16_t frameIndex,
                                 bool *cameraCaptured = nullptr) {
  if (cameraCaptured != nullptr) {
    *cameraCaptured = false;
  }
  camera_fb_t *frame = esp_camera_fb_get();
  if (frame == nullptr) {
    incrementCounter(&CounterSnapshot::cameraFailures);
    Serial.println("Camera capture failed");
    return false;
  }
  if (cameraCaptured != nullptr) {
    *cameraCaptured = true;
  }
  const uint64_t captureMillis = uptimeMillis();
  const bool accepted = enqueueFrame(frame->buf, frame->len, eventId, frameIndex, captureMillis);
  esp_camera_fb_return(frame);
  return accepted;
}

static bool calculateGrid(uint8_t grid[GRID_CELLS]) {
  camera_fb_t *frame = esp_camera_fb_get();
  if (frame == nullptr) {
    incrementCounter(&CounterSnapshot::cameraFailures);
    return false;
  }
  const bool converted = fmt2rgb888(frame->buf, frame->len, frame->format, rgbScratch);
  esp_camera_fb_return(frame);
  if (!converted) {
    Serial.println("Could not decode detection JPEG");
    incrementCounter(&CounterSnapshot::cameraFailures);
    return false;
  }

  const uint32_t roiLeft = (static_cast<uint32_t>(settings.roiX) * DETECT_WIDTH) / 1000;
  const uint32_t roiTop = (static_cast<uint32_t>(settings.roiY) * DETECT_HEIGHT) / 1000;
  const uint32_t roiWidth = max<uint32_t>(1, (static_cast<uint32_t>(settings.roiW) * DETECT_WIDTH) / 1000);
  const uint32_t roiHeight = max<uint32_t>(1, (static_cast<uint32_t>(settings.roiH) * DETECT_HEIGHT) / 1000);
  const uint32_t roiRight = min<uint32_t>(DETECT_WIDTH, roiLeft + roiWidth);
  const uint32_t roiBottom = min<uint32_t>(DETECT_HEIGHT, roiTop + roiHeight);

  for (uint8_t row = 0; row < GRID_ROWS; ++row) {
    for (uint8_t column = 0; column < GRID_COLUMNS; ++column) {
      const uint32_t x0 = roiLeft + (roiRight - roiLeft) * column / GRID_COLUMNS;
      const uint32_t x1 = roiLeft + (roiRight - roiLeft) * (column + 1) / GRID_COLUMNS;
      const uint32_t y0 = roiTop + (roiBottom - roiTop) * row / GRID_ROWS;
      const uint32_t y1 = roiTop + (roiBottom - roiTop) * (row + 1) / GRID_ROWS;
      uint32_t sum = 0;
      uint32_t samples = 0;
      for (uint32_t y = y0; y < y1; y += 2) {
        for (uint32_t x = x0; x < x1; x += 2) {
          const size_t offset = (static_cast<size_t>(y) * DETECT_WIDTH + x) * 3;
          const uint8_t red = rgbScratch[offset];
          const uint8_t green = rgbScratch[offset + 1];
          const uint8_t blue = rgbScratch[offset + 2];
          sum += (77U * red + 150U * green + 29U * blue) >> 8;
          samples++;
        }
      }
      grid[row * GRID_COLUMNS + column] = samples == 0 ? 0 : sum / samples;
    }
  }
  return true;
}

static int16_t medianDifference(const int16_t values[GRID_CELLS]) {
  int16_t sorted[GRID_CELLS];
  memcpy(sorted, values, sizeof(sorted));
  for (uint8_t i = 1; i < GRID_CELLS; ++i) {
    const int16_t value = sorted[i];
    int8_t j = static_cast<int8_t>(i) - 1;
    while (j >= 0 && sorted[j] > value) {
      sorted[j + 1] = sorted[j];
      --j;
    }
    sorted[j + 1] = value;
  }
  return sorted[GRID_CELLS / 2];
}

static bool hasChangedCluster(const bool changed[GRID_CELLS]) {
  bool visited[GRID_CELLS] = {false};
  uint8_t stack[GRID_CELLS];
  for (uint8_t cell = 0; cell < GRID_CELLS; ++cell) {
    if (!changed[cell] || visited[cell]) {
      continue;
    }
    uint8_t stackSize = 0;
    uint8_t clusterSize = 0;
    stack[stackSize++] = cell;
    visited[cell] = true;
    while (stackSize > 0) {
      const uint8_t current = stack[--stackSize];
      clusterSize++;
      const uint8_t row = current / GRID_COLUMNS;
      const uint8_t column = current % GRID_COLUMNS;
      const int8_t neighbors[4] = {
          row > 0 ? static_cast<int8_t>(current - GRID_COLUMNS) : -1,
          row + 1 < GRID_ROWS ? static_cast<int8_t>(current + GRID_COLUMNS) : -1,
          column > 0 ? static_cast<int8_t>(current - 1) : -1,
          column + 1 < GRID_COLUMNS ? static_cast<int8_t>(current + 1) : -1};
      for (uint8_t i = 0; i < 4; ++i) {
        const int8_t next = neighbors[i];
        if (next >= 0 && changed[next] && !visited[next]) {
          visited[next] = true;
          stack[stackSize++] = static_cast<uint8_t>(next);
        }
      }
    }
    if (clusterSize >= 3) {
      return true;
    }
  }
  return false;
}

static bool detectMotion() {
  uint8_t currentGrid[GRID_CELLS];
  if (!calculateGrid(currentGrid)) {
    return false;
  }
  if (!previousGridValid) {
    memcpy(previousGrid, currentGrid, sizeof(previousGrid));
    previousGridValid = true;
    return false;
  }

  int16_t differences[GRID_CELLS];
  for (uint8_t i = 0; i < GRID_CELLS; ++i) {
    differences[i] = static_cast<int16_t>(currentGrid[i]) - previousGrid[i];
  }
  memcpy(previousGrid, currentGrid, sizeof(previousGrid));
  const int16_t commonChange = medianDifference(differences);
  const uint8_t commonMagnitude = static_cast<uint8_t>(abs(commonChange));
  bool changed[GRID_CELLS] = {false};
  uint8_t changedCount = 0;
  uint8_t sameDirectionCount = 0;

  for (uint8_t i = 0; i < GRID_CELLS; ++i) {
    const int16_t compensated = differences[i] - commonChange;
    if (abs(compensated) >= settings.motionThreshold) {
      changed[i] = true;
      changedCount++;
    }
    if (abs(differences[i]) >= settings.motionThreshold &&
        ((differences[i] >= 0) == (commonChange >= 0))) {
      sameDirectionCount++;
    }
  }

  const uint8_t globalLimit = max<uint8_t>(1, (GRID_CELLS * settings.globalChangeLimit) / 100);
  const bool globalLightChange = commonMagnitude >= settings.motionThreshold &&
                                 sameDirectionCount >= globalLimit;
  const bool motion = !globalLightChange && changedCount >= 3 && hasChangedCluster(changed);
  triggerHistory[triggerHistoryIndex] = motion ? 1 : 0;
  triggerHistoryIndex = (triggerHistoryIndex + 1) % 3;
  const uint8_t positiveComparisons =
      triggerHistory[0] + triggerHistory[1] + triggerHistory[2];
  return positiveComparisons >= settings.triggerFrames;
}

static uint64_t beginEvent() {
  xSemaphoreTake(eventMutex, portMAX_DELAY);
  const uint64_t eventId = nextEventId++;
  preferences.putULong64("next_event_id", nextEventId);
  xSemaphoreTake(counterMutex, portMAX_DELAY);
  counters.events++;
  counters.detections++;
  counters.lastTriggerMillis = millis();
  xSemaphoreGive(counterMutex);
  xSemaphoreGive(eventMutex);
  Serial.printf("Motion detected; event %llu\n", static_cast<unsigned long long>(eventId));
  return eventId;
}

static void cameraTask(void *parameter) {
  (void)parameter;
  uint32_t cooldownUntil = 0;
  uint64_t activeEventId = 0;
  uint16_t eventFrameIndex = 0;
  uint32_t eventStartedAt = 0;
  uint32_t nextEventFrameAt = 0;
  uint8_t consecutiveCameraErrors = 0;

  for (;;) {
    const uint32_t now = millis();
    if (activeEventId != 0) {
      const uint32_t eventDuration = static_cast<uint32_t>(settings.eventSeconds) * 1000;
      if (static_cast<int32_t>(now - eventStartedAt) >= static_cast<int32_t>(eventDuration)) {
        xSemaphoreTake(cameraMutex, portMAX_DELAY);
        const bool restored = setCameraFrameSize(FRAMESIZE_QVGA);
        xSemaphoreGive(cameraMutex);
        if (!restored) {
          Serial.println("Failed to restore detection frame size");
        }
        activeEventId = 0;
        cooldownUntil = now + settings.cooldownMs;
        previousGridValid = false;
        memset(triggerHistory, 0, sizeof(triggerHistory));
        vTaskDelay(pdMS_TO_TICKS(20));
        continue;
      }
      if (static_cast<int32_t>(now - nextEventFrameAt) >= 0) {
        xSemaphoreTake(cameraMutex, portMAX_DELAY);
        bool cameraCaptured = false;
        enqueueCapturedFrame(activeEventId, eventFrameIndex, &cameraCaptured);
        xSemaphoreGive(cameraMutex);
        if (cameraCaptured) {
          eventFrameIndex++;
        }
        consecutiveCameraErrors = cameraCaptured ? 0 : consecutiveCameraErrors + 1;
        nextEventFrameAt = now + (1000 / settings.eventFps);
      }
      if (consecutiveCameraErrors >= 5) {
        Serial.println("Repeated camera failures; restarting device");
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP.restart();
      }
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    if (static_cast<int32_t>(now - cooldownUntil) < 0) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    const uint32_t failuresBeforeDetection = getCounters().cameraFailures;
    xSemaphoreTake(cameraMutex, portMAX_DELAY);
    const bool motion = detectMotion();
    xSemaphoreGive(cameraMutex);
    const uint32_t failuresAfterDetection = getCounters().cameraFailures;
    if (failuresAfterDetection > failuresBeforeDetection) {
      consecutiveCameraErrors++;
    } else {
      consecutiveCameraErrors = 0;
    }
    if (consecutiveCameraErrors >= 5) {
      Serial.println("Repeated camera failures during detection; restarting device");
      vTaskDelay(pdMS_TO_TICKS(500));
      ESP.restart();
    }
    if (motion) {
      const uint64_t eventId = beginEvent();
      xSemaphoreTake(cameraMutex, portMAX_DELAY);
      const bool configured = setCameraFrameSize(EVENT_FRAME_SIZE);
      xSemaphoreGive(cameraMutex);
      if (configured) {
        activeEventId = eventId;
        eventFrameIndex = 0;
        eventStartedAt = millis();
        nextEventFrameAt = eventStartedAt;
      } else {
        cooldownUntil = millis() + settings.cooldownMs;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(max<uint32_t>(1, 1000 / settings.detectFps)));
  }
}

static void uploaderTask(void *parameter) {
  (void)parameter;
  const uint32_t retrySeconds[] = {1, 2, 5, 10, 30, 60};
  uint8_t retryIndex = 0;

  for (;;) {
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - heartbeatAt) >= 0) {
      postHeartbeat();
      heartbeatAt = now + 60000;
    }

    xSemaphoreTake(queueMutex, portMAX_DELAY);
    QueuedFrame *entry = queueHead;
    xSemaphoreGive(queueMutex);
    if (entry == nullptr) {
      vTaskDelay(pdMS_TO_TICKS(250));
      continue;
    }
    if (!WiFi.isConnected()) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    int status = 0;
    const bool uploaded = uploadFrame(entry, status);
    setBackendStatus(status);
    if (uploaded) {
      removeQueueHead(entry);
      incrementCounter(&CounterSnapshot::framesUploaded);
      retryIndex = 0;
      continue;
    }

    incrementCounter(&CounterSnapshot::uploadFailures);
    Serial.printf("Frame upload failed (HTTP %d)\n", status);
    if (status >= 400 && status < 500) {
      vTaskDelay(pdMS_TO_TICKS(300000));
    } else {
      const uint32_t delaySeconds = retrySeconds[min<uint8_t>(retryIndex, 5)];
      if (retryIndex < 5) {
        retryIndex++;
      }
      vTaskDelay(pdMS_TO_TICKS(delaySeconds * 1000));
    }
  }
}

static String statusJson() {
  const QueueSnapshot queue = getQueueSnapshot();
  const CounterSnapshot stats = getCounters();
  String json = "{\"camera_id\":\"" + jsonEscape(settings.cameraId) +
                "\",\"ip\":\"" + (WiFi.isConnected() ? WiFi.localIP().toString() : String("")) +
                "\",\"rssi\":" + String(WiFi.isConnected() ? WiFi.RSSI() : 0) +
                ",\"uptime_s\":" + uint64String(uptimeMillis() / 1000) +
                ",\"queue_frames\":" + String(queue.frames) +
                ",\"queue_bytes\":" + String(queue.bytes) +
                ",\"detections\":" + String(stats.detections) +
                ",\"events\":" + String(stats.events) +
                ",\"frames_captured\":" + String(stats.framesCaptured) +
                ",\"frames_uploaded\":" + String(stats.framesUploaded) +
                ",\"dropped_frames\":" + String(stats.droppedFrames) +
                ",\"upload_failures\":" + String(stats.uploadFailures) +
                ",\"camera_failures\":" + String(stats.cameraFailures) +
                ",\"last_trigger_ms\":" + String(stats.lastTriggerMillis) +
                ",\"last_backend_status\":" + String(stats.lastBackendStatus) + "}";
  return json;
}

static String configJson() {
  String json = "{\"camera_id\":\"" + jsonEscape(settings.cameraId) +
                "\",\"wifi_ssid\":\"" + jsonEscape(settings.wifiSsid) +
                "\",\"backend_url\":\"" + jsonEscape(settings.backendUrl) +
                "\",\"roi\":{\"x\":" + String(settings.roiX) +
                ",\"y\":" + String(settings.roiY) +
                ",\"w\":" + String(settings.roiW) +
                ",\"h\":" + String(settings.roiH) +
                "},\"jpeg_quality\":" + String(settings.jpegQuality) +
                ",\"motion_threshold\":" + String(settings.motionThreshold) +
                ",\"global_change_limit\":" + String(settings.globalChangeLimit) +
                ",\"detect_fps\":" + String(settings.detectFps) +
                ",\"trigger_frames\":" + String(settings.triggerFrames) +
                ",\"event_seconds\":" + String(settings.eventSeconds) +
                ",\"event_fps\":" + String(settings.eventFps) +
                ",\"cooldown_ms\":" + String(settings.cooldownMs) + "}";
  return json;
}

static String setupPage() {
  String page = R"HTML(<!doctype html><html lang="nl"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>WildCam setup</title>
<style>body{font:16px sans-serif;max-width:900px;margin:2rem auto;padding:0 1rem;background:#f4f6f7;color:#17202a}
section{background:white;padding:1rem;margin:1rem 0;border-radius:8px}label{display:block;margin:.6rem 0}
input{padding:.45rem;max-width:100%}button{padding:.6rem 1rem}#view{position:relative;width:100%;max-width:800px}
#view img{width:100%;display:block}#roi{position:absolute;border:2px solid #00ff70;box-sizing:border-box;pointer-events:none}
pre{white-space:pre-wrap}</style></head><body><h1>WildCam: )HTML";
  page += settings.cameraId;
  page += R"HTML(</h1><section><h2>Livebeeld</h2><div id="view"><img src="/stream"><div id="roi"></div></div></section>
<section><h2>Configuratie</h2><form method="post" action="/api/config">
<label>Camera-ID <input name="camera_id" required></label>
<label>WiFi-SSID <input name="wifi_ssid"></label>
<label>WiFi-wachtwoord <input name="wifi_password" type="password"></label>
<label>Backend-URL <input name="backend_url" required></label>
<label>ROI x (0-1000) <input name="roi_x" type="number" min="0" max="999" required></label>
<label>ROI y (0-1000) <input name="roi_y" type="number" min="0" max="999" required></label>
<label>ROI breedte <input name="roi_w" type="number" min="1" max="1000" required></label>
<label>ROI hoogte <input name="roi_h" type="number" min="1" max="1000" required></label>
<label>JPEG-kwaliteit (10-63) <input name="jpeg_quality" type="number" min="10" max="63" required></label>
<label>Bewegingsdrempel <input name="motion_threshold" type="number" min="1" max="100" required></label>
<label>Globale lichtlimiet (%) <input name="global_change_limit" type="number" min="10" max="100" required></label>
<label>Detectieframes per seconde <input name="detect_fps" type="number" min="1" max="10" required></label>
<label>Triggers in de laatste 3 vergelijkingen <input name="trigger_frames" type="number" min="1" max="3" required></label>
<label>Eventduur (seconden) <input name="event_seconds" type="number" min="1" max="60" required></label>
<label>Eventframes per seconde <input name="event_fps" type="number" min="1" max="10" required></label>
<label>Cooldown (milliseconden) <input name="cooldown_ms" type="number" min="500" max="60000" required></label>
<button type="submit">Opslaan en herstarten</button></form></section>
<section><h2>Status</h2><pre id="status">Laden…</pre>
<button onclick="fetch('/api/test-upload',{method:'POST'}).then(r=>r.text()).then(alert)">Testupload</button>
<button onclick="fetch('/api/reboot',{method:'POST'})">Herstart</button></section>
<script>fetch('/api/config').then(r=>r.json()).then(c=>{for(const n of ['camera_id','wifi_ssid','backend_url','jpeg_quality','motion_threshold','global_change_limit','detect_fps','trigger_frames','event_seconds','event_fps','cooldown_ms'])document.querySelector('[name='+n+']').value=c[n];for(const n of ['x','y','w','h'])document.querySelector('[name=roi_'+n+']').value=c.roi[n];document.getElementById('roi').style.cssText='left:'+c.roi.x/10+'%;top:'+c.roi.y/10+'%;width:'+c.roi.w/10+'%;height:'+c.roi.h/10+'%';const preview=document.querySelector('#view img');preview.onerror=()=>setTimeout(()=>preview.src='/stream?'+Date.now(),1000)});setInterval(()=>fetch('/api/status').then(r=>r.json()).then(s=>document.getElementById('status').textContent=JSON.stringify(s,null,2)),3000)</script>
</body></html>)HTML";
  return page;
}

static void handleConfigPost() {
  CameraSettings candidate = settings;
  if (webServer.hasArg("camera_id")) {
    candidate.cameraId = webServer.arg("camera_id");
  }
  if (webServer.hasArg("wifi_ssid")) {
    candidate.wifiSsid = webServer.arg("wifi_ssid");
  }
  if (webServer.hasArg("wifi_password") && !webServer.arg("wifi_password").isEmpty()) {
    candidate.wifiPassword = webServer.arg("wifi_password");
  }
  if (webServer.hasArg("backend_url")) {
    candidate.backendUrl = webServer.arg("backend_url");
  }

  long x, y, width, height, quality, threshold, globalLimit;
  long detectFps, triggerFrames, eventSeconds, eventFps, cooldownMs;
  const bool valid =
      validCameraId(candidate.cameraId) &&
      candidate.backendUrl.startsWith("http://") &&
      parseBoundedInteger(webServer.arg("roi_x"), 0, 999, x) &&
      parseBoundedInteger(webServer.arg("roi_y"), 0, 999, y) &&
      parseBoundedInteger(webServer.arg("roi_w"), 1, 1000, width) &&
      parseBoundedInteger(webServer.arg("roi_h"), 1, 1000, height) &&
      x + width <= 1000 && y + height <= 1000 &&
      parseBoundedInteger(webServer.arg("jpeg_quality"), 10, 63, quality) &&
      parseBoundedInteger(webServer.arg("motion_threshold"), 1, 100, threshold) &&
      parseBoundedInteger(webServer.arg("global_change_limit"), 10, 100, globalLimit) &&
      parseBoundedInteger(webServer.arg("detect_fps"), 1, 10, detectFps) &&
      parseBoundedInteger(webServer.arg("trigger_frames"), 1, 3, triggerFrames) &&
      parseBoundedInteger(webServer.arg("event_seconds"), 1, 60, eventSeconds) &&
      parseBoundedInteger(webServer.arg("event_fps"), 1, 10, eventFps) &&
      parseBoundedInteger(webServer.arg("cooldown_ms"), 500, 60000, cooldownMs);
  if (!valid) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"Ongeldige configuratie\"}");
    return;
  }

  settings = candidate;
  settings.roiX = x;
  settings.roiY = y;
  settings.roiW = width;
  settings.roiH = height;
  settings.jpegQuality = quality;
  settings.motionThreshold = threshold;
  settings.globalChangeLimit = globalLimit;
  settings.detectFps = detectFps;
  settings.triggerFrames = triggerFrames;
  settings.eventSeconds = eventSeconds;
  settings.eventFps = eventFps;
  settings.cooldownMs = cooldownMs;
  preferences.putString("camera_id", settings.cameraId);
  preferences.putString("wifi_ssid", settings.wifiSsid);
  preferences.putString("wifi_password", settings.wifiPassword);
  preferences.putString("backend_url", settings.backendUrl);
  preferences.putUShort("roi_x", settings.roiX);
  preferences.putUShort("roi_y", settings.roiY);
  preferences.putUShort("roi_w", settings.roiW);
  preferences.putUShort("roi_h", settings.roiH);
  preferences.putUChar("jpeg_quality", settings.jpegQuality);
  preferences.putUChar("motion_threshold", settings.motionThreshold);
  preferences.putUChar("global_limit", settings.globalChangeLimit);
  preferences.putUChar("detect_fps", settings.detectFps);
  preferences.putUChar("trigger_frames", settings.triggerFrames);
  preferences.putUChar("event_seconds", settings.eventSeconds);
  preferences.putUChar("event_fps", settings.eventFps);
  preferences.putUShort("cooldown_ms", settings.cooldownMs);
  webServer.send(200, "text/html; charset=utf-8",
                 "<!doctype html><meta charset=utf-8><p>Configuratie opgeslagen. Herstarten…</p>");
  delay(700);
  ESP.restart();
}

static void handleSnapshot() {
  xSemaphoreTake(cameraMutex, portMAX_DELAY);
  camera_fb_t *frame = esp_camera_fb_get();
  if (frame == nullptr) {
    xSemaphoreGive(cameraMutex);
    webServer.send(503, "text/plain", "Camera niet beschikbaar");
    return;
  }
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.setContentLength(frame->len);
  webServer.send(200, "image/jpeg", "");
  WiFiClient client = webServer.client();
  client.write(frame->buf, frame->len);
  esp_camera_fb_return(frame);
  xSemaphoreGive(cameraMutex);
}

static void handleStream() {
  WiFiClient client = webServer.client();
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "multipart/x-mixed-replace; boundary=frame", "");
  const uint32_t started = millis();
  while (client.connected() && millis() - started < 10000) {
    xSemaphoreTake(cameraMutex, portMAX_DELAY);
    camera_fb_t *frame = esp_camera_fb_get();
    if (frame == nullptr) {
      xSemaphoreGive(cameraMutex);
      delay(100);
      continue;
    }
    char header[96];
    const int headerLength = snprintf(header, sizeof(header),
                                      "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                                      static_cast<unsigned>(frame->len));
    client.write(reinterpret_cast<const uint8_t *>(header), headerLength);
    client.write(frame->buf, frame->len);
    client.write(reinterpret_cast<const uint8_t *>("\r\n"), 2);
    esp_camera_fb_return(frame);
    xSemaphoreGive(cameraMutex);
    delay(180);
  }
  client.stop();
}

static bool enqueueTestFrame() {
  xSemaphoreTake(cameraMutex, portMAX_DELAY);
  const uint64_t eventId = beginEvent();
  const bool accepted = enqueueCapturedFrame(eventId, 0);
  xSemaphoreGive(cameraMutex);
  return accepted;
}

static void configureWebServer() {
  webServer.on("/", HTTP_GET, []() {
    webServer.send(200, "text/html; charset=utf-8", setupPage());
  });
  webServer.on("/snapshot.jpg", HTTP_GET, handleSnapshot);
  webServer.on("/stream", HTTP_GET, handleStream);
  webServer.on("/api/status", HTTP_GET, []() {
    webServer.send(200, "application/json", statusJson());
  });
  webServer.on("/api/config", HTTP_GET, []() {
    webServer.send(200, "application/json", configJson());
  });
  webServer.on("/api/config", HTTP_POST, handleConfigPost);
  webServer.on("/api/reboot", HTTP_POST, []() {
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(300);
    ESP.restart();
  });
  webServer.on("/api/test-upload", HTTP_POST, []() {
    const bool accepted = enqueueTestFrame();
    webServer.send(accepted ? 202 : 503, "application/json",
                   accepted ? "{\"ok\":true,\"queued\":true}" :
                              "{\"ok\":false,\"error\":\"Frame kon niet aan wachtrij worden toegevoegd\"}");
  });
  webServer.onNotFound([]() {
    webServer.send(404, "text/plain", "Niet gevonden");
  });
  webServer.begin();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nWildCam ESP32-CAM firmware 1.0.0");
  Serial.printf("Reset reason: %d\n", static_cast<int>(esp_reset_reason()));

  loadSettings();
  cameraMutex = xSemaphoreCreateMutex();
  queueMutex = xSemaphoreCreateMutex();
  counterMutex = xSemaphoreCreateMutex();
  eventMutex = xSemaphoreCreateMutex();
  if (cameraMutex == nullptr || queueMutex == nullptr || counterMutex == nullptr ||
      eventMutex == nullptr) {
    Serial.println("Could not create required mutexes");
    delay(1000);
    ESP.restart();
  }

  bootId = esp_random();
  Serial.printf("Camera ID: %s; boot ID: %08lx\n",
                settings.cameraId.c_str(), static_cast<unsigned long>(bootId));
  Serial.printf("PSRAM: %u bytes\n", static_cast<unsigned>(ESP.getPsramSize()));
  if (!psramFound()) {
    Serial.println("PSRAM is required; refusing to run without bounded frame storage");
    delay(1000);
    ESP.restart();
  }

  if (!initializeCamera()) {
    incrementCounter(&CounterSnapshot::cameraFailures);
    Serial.println("Camera unavailable; restarting in 5 seconds");
    delay(5000);
    ESP.restart();
  }
  rgbScratch = static_cast<uint8_t *>(ps_malloc(DETECT_WIDTH * DETECT_HEIGHT * 3));
  if (rgbScratch == nullptr) {
    Serial.println("Could not allocate motion-detection buffer in PSRAM");
    delay(1000);
    ESP.restart();
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  if (!settings.wifiSsid.isEmpty()) {
    WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPassword.c_str());
    wifiStartedAt = millis();
  } else {
    Serial.println("WiFi is not configured; setup access point will start.");
  }
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  configureWebServer();
  heartbeatAt = millis() + 10000;
  xTaskCreatePinnedToCore(cameraTask, "wildcam-camera", 8192, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(uploaderTask, "wildcam-uploader", 8192, nullptr, 1, nullptr, 0);
  Serial.println("Camera and uploader tasks started");
}

void loop() {
  webServer.handleClient();
  if (WiFi.isConnected()) {
    if (!mdnsStarted) {
      if (MDNS.begin(settings.cameraId.c_str())) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("WiFi connected: IP %s, RSSI %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        Serial.printf("Web UI: http://%s.local/\n", settings.cameraId.c_str());
        mdnsStarted = true;
      } else {
        Serial.println("mDNS startup failed");
      }
    }
    if (setupApStarted) {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      setupApStarted = false;
    }
  } else {
    if (mdnsStarted) {
      MDNS.end();
    }
    mdnsStarted = false;
    if (!settings.wifiSsid.isEmpty() &&
        static_cast<int32_t>(millis() - wifiRetryAt) >= 0) {
      Serial.println("WiFi disconnected; retrying");
      WiFi.reconnect();
      wifiRetryAt = millis() + 10000;
    }
    const bool noCredentials = settings.wifiSsid.isEmpty();
    const bool connectionTimedOut = !noCredentials && wifiStartedAt != 0 &&
                                    millis() - wifiStartedAt >= 30000;
    if (!setupApStarted && (noCredentials || connectionTimedOut)) {
      char password[9];
      snprintf(password, sizeof(password), "%08lx",
               static_cast<unsigned long>(esp_random()));
      const String apName = "WildCam-Setup-" + settings.cameraId;
      WiFi.mode(WIFI_AP_STA);
      if (WiFi.softAP(apName.c_str(), password)) {
        setupApStarted = true;
        Serial.printf("Setup AP: %s  password: %s  address: %s\n",
                      apName.c_str(), password, WiFi.softAPIP().toString().c_str());
      } else {
        Serial.println("Could not start setup access point");
      }
    }
  }
  delay(2);
}
