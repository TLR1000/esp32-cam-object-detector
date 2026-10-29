/**
 * Library.ino - Externe bibliotheken en includes voor WildCam ESP32-CAM Arduino sketch
 * 
 * Deze library bevat:
 *   - Camera library (ESP32Camera of equivalent)
 *   - WiFi connectivity handler  
 *   - Motion detection logic
 *   - Uploader client
 */

// Voor Arduino IDE gebruik, vereist u deze bibliotheken:
// 1. ESP32 by Espressif Systems (standaard met Arduino board support package)
// 2. ESP32Camera library (https://github.com/links2004/ESP32-Camera-library)
//    OF een alternatieve camera library

#include "Arduino.h"
#include "SPIFFS.h"        // Voor PSRAM buffering
#include "Preferences.h"   // Voor NVS configuratie opslag

// Camera library - kies één van de onderstaande opties:
// #include "ESP32Camera.h"    // Als deze beschikbaar is
// #include "ArduinoCamera.h"  // Alternatieve optie

// WiFi connectivity handler
#include <WiFi.h>          // Standaard Arduino WiFi library
#include <HTTPClient.h>    // Voor backend API uploads

// Voor motion detection:
// We gebruiken de ingebouwde frame buffer van camera library
// Custom logic in separate .cpp files (zie platformio versie)

/**
 * Data types en structs voor firmware configuratie
 */

typedef struct {
    uint32_t width;        // Frame breedte
    uint32_t height;       // Frame hoogte  
    uint32_t fps;          // Frames per second target
    bool psram_enabled;    // PSRAM actief/uit
} CameraConfig;

/**
 * Motion detection settings:
 */
const int MOTION_THRESHOLD = 50;   // Veranderingstrend in ROI pixels
const int MOTION_COOLDOWN = 2000;  // Milliseconde cooldown na trigger

// Backend configuratie (zie platformio.ini voor complete lijst)
extern const char* BACKEND_URL;     // Upload endpoint
extern const uint8_t BOOT_ID[16];   // Voor event-ID generatie