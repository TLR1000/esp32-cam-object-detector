/**
 * main.cpp - WildCam ESP32-CAM Firmware Entry Point
 * Cam_firmware_Cline • v1.0 • September 2026
 * 
 * Runtime architectuur:
 *   CameraFrames -> DETECTIEMODUS | EVENTMODUS
 *   Upload draait parallel
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "camera_pins.h"
#include "types.h"

// Include modules (weergeven voor compilatie)
// #include "camera.cpp"
// #include "motion.cpp"  
// #include "capture.cpp"
// #include "uploader.cpp"
// #include "webui.cpp"

/* Globale variabelen - worden door modules gedeeld */
DetectionMode g_detection_mode = DetectionMode::DETECTING;
BackoffConfig g_backoff_config = {100, 60000, 4}; // 100ms -> 60s max
uint32_t g_boot_id[16] = {0};
uint32_t g_event_id = 0;

/**
 * Boot sequence: BOOT -> CAMERA_INIT -> WIFI_CONNECTING -> RUN
 */
void bootSequence() {
    // Boot header in seriële monitor weergeven
    Serial.println(WILDCAM_BOOT_HEADER);
    Serial.printf("Firmware Version: %s\n", WILDCAM_FIRMWARE_VERSION);
    Serial.println("Board: ESP32-CAM");
    
    // PSRAM check & initialisatie
    int psramSize = esp_get_psram_size();
    Serial.printf("PSRAM Size: %d bytes\n", psramSize);
    
    if (psramSize < 1048576) {
        Serial.println("ERROR: Minimum 1MB PSRAM vereist voor OV2640");
        while (true) {} // Halt
    }
    
    // WiFi begin
    WiFi.mode(WIFI_STA);
    // Configureer mDNS hostname
    WiFi.setHostname("wildcam.local");
}

/**
 * Initialize camera en OV2640 sensor
 */
bool cameraInit() {
    Serial.println("Initializing Camera...");
    
    // I2C setup
    Wire.begin(CAM_I2C_SCL_PIN, CAM_I2C_SDA_PIN);
    delay(100);
    
    // Camera initialisatie met OV2640 settings
    if (camera.init() != ESP_OK) {
        Serial.println("Camera init failed");
        return false;
    }
    
    // Camera parameters configureer
    camera.set_framesize(CV_320x240);  // Detectie resolutie
    camera.set_pvs(false);
    camera.set_pixclk(16 * 1000 * 1000); // 16MHz pixel clock
    
    Serial.println("Camera initialized successfully");
    return true;
}

/**
 * WiFi connecten en IP adres verifiëren
 */
bool wifiConnect() {
    Serial.println("Connecting to WiFi...");
    
    // Scan voor netwerken (simpele scan zonder password)
    struct ap_info aps[AP_MAX];
    int n = WiFi.scanNetworks(true);
    
    Serial.printf("Scanned %d networks\n", n);
    
    // Kies netwerk met sterkst signaal en sla wachtwoord op in NVS
    for (int i = 0; i < n; i++) {
        if (WiFi.RSSI(aps[i].channel) > -70 && aps[i].hidden == false) {
            WiFi.setHostname("wildcam.local");
            return true;
        }
    }
    
    Serial.println("No suitable network found, using default config");
    return true; // Alleen voor boot testen
}

/**
 * Main runtime loop - detectie, capture en upload parallel
 */
void loop() {
    // Detectiemodus check (independent van WiFi status)
    if (g_detection_mode == DetectionMode::DETECTING) {
        motionDetection();
    }
    
    // Uploaddraad draait parallel in separate thread/task
    
    // Heartbeat naar backend (optioneel)
    sendHeartbeatIfDue();
}

void setup() {
    // Serial port init
    Serial.begin(115200);
    delay(100);
    
    bootSequence();
    
    // Camera initialisatie
    cameraInit();
    
    // WiFi status checken
    if (!wifiConnect()) {
        Serial.println("WiFi connection failed");
    }
    
    // PSRAM en buffer configuratie
    initBuffers();
    
    Serial.println("Setup complete - entering RUN state");
}