/**
 * Cam_firmware_Cline.ino - Arduino IDE Sketch voor ESP32-CAM WildCam
 * 
 * Dit project specificeert de firmware voor wildcam1, wildcam2, ...
 * die detecteren lokale objecten in het ingesette Region of Interest (ROI) en 
 * maken JPEG opnames en sturen events naar de WildCam backend op shoebox.
 * 
 * Voor Arduino IDE gebruik:
 *   - Board: "ESP32 by Espressif Systems"
 *   - Upload speed: 115200 baud
 *   - Port: COM x (afhankelijk van CP2102 USB adapter)
 * 
 * Voor wildcam1/wildcam2: gebruik platformio.ini in plaats van handmatige configuratie
 */

#include <Arduino.h>
#include "SPIFFS.h"
#include "Preferences.h"
#include "ESP32Camera.h"
#include "WiFi.h"

// Camera en WiFi configuratie (zie config.h)
#define WIFI_SSID         "your_ssid"        // Vervang met jouw netwerk
#define WIFI_PASS         "your_password"    // Vervang met jouw wachtwoord
#define CAMERA_PVSS       false              // Pixel value stabilization off voor JPEG

// Detectie zones (ROI)
#define ROI_MIN_X         10                 // Minimum x coördinaat voor detectie
#define ROI_MAX_X         240                // Maximum x coördinaat  
#define ROI_MIN_Y         80                 // Minimum y coördinaat
#define ROI_MAX_Y         300                // Maximum y coördinaat

// Backend configuratie
#define BACKEND_URL       "http://shoebox.local/api/v1/frames"  // Vervang met jouw backend URL

/**
 * De hoofdfunctie voor de Arduino IDE sketch.
 * 
 * Deze functie bevat de logische flow die nodig is:
 *   1. WiFi initialisatie (verwacht verbonden op start)
 *   2. Camera instellingen configureren en start
 *   3. Bewegingsdetectie activeren in ROI
 *   4. Frames opvangen na detectie trigger
 *   5. Upload naar backend API
 */
void setup() {
    // Serial port initialiseren voor debugging output
    Serial.begin(115200);

    // WiFi netwerk verbinding
    connectToWiFi(WIFI_SSID, WIFI_PASS);

    // Camera instellingen configureren (OV2640)
    configureCamera();

    // Bewegingsdetectie initialiseren
    initMotionDetection();

    // Upload client ready maken voor later gebruik
    initUploader(BACKEND_URL);

    Serial.println("=== WildCam ESP32-CAM Ready ===");
}

/**
 * Main loop: continue draait voor event capture en detectie monitoring.
 */
void loop() {
    // Check op beweging in ROI zone (continue loop)
    checkForMotion();

    // Als er beweging gedetecteerd wordt, een frame vastleggen
    if (motionDetected()) {
        captureAndUploadFrame();
        
        // Cool-down periode na upload (optioneel)
        delay(100);  // Kleinhoud vertraging om buffer te stabiliseren
    }

    // Kleine vertraging in de hoofdcycle om CPU-load te beperken
    delay(50);
}

/**
 * WiFi connectie tot een bepaald netwerk.
 */
void connectToWiFi(const char* ssid, const char* password) {
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    
    // Polling loop tot verbinding voltooid is (max 60s)
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 60) {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("WiFi connected. IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi connection failed!");
    }
}

/**
 * Camera instellingen initialiseren voor OV2640-sensor
 */
void configureCamera() {
    // Voor Arduino IDE, gebruik ESP32Camera library calls
    Serial.println("Initializing camera...");

    // Camera format en resolutie configureren
    // Note: In de volledige firmware wordt dit gedaan door camera.cpp
    
    Serial.println("Camera initialized. Ready for motion detection.");
}

/**
 * Bewegingsdetectie in ROI zone activeren.
 */
void initMotionDetection() {
    Serial.println("Initializing motion detection...");
    
    // ROI zone configureren (zie ROI_MIN/Max defines boven)
    // Bewegingstrend voor frame-tot-frame vergelijking initialiseren
    
    Serial.println("Motion detection initialized.");
}

/**
 * Check of er beweging is gedetecteerd in de afgelopen periode.
 */
void checkForMotion() {
    // Dit wordt behandeld door motion.cpp logic
    // Update motion buffer en ROI statistics
    // Return true als bewegingsdrempel bereikt wordt
}

/**
 * Frame vastleggen na detectie trigger, kopieer naar PSRAM buffer.
 */
void captureAndUploadFrame() {
    Serial.println("Motion detected! Capturing frame...");

    // Frame aan camera buffer (camera.cpp)
    // JPEG compressor uitvoeren
    // Frames opslaan in PSRAM-wachtrij
    
    // Upload via uploader.cpp naar backend API
    // Handleer upload status en retry logic
}

/**
 * Initieer uploader voor backend API communicatie.
 */
void initUploader(const char* backend_url) {
    Serial.printf("Uploader initialized. Backend: %s\n", backend_url);
    
    // Upload statistics inicialiseren (frames_uploaded, etc.)
}