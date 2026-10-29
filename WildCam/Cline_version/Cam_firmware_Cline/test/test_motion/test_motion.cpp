/**
 * test_motion.cpp - Tests voor bewegingsdetectie
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#include <Arduino.h>
#include "motion.h"
#include "types.h"

/**
 * Testcase: Detectie van object dat ROI binnentreedt
 */
void testObjectEntersROI() {
    Serial.println("Test: Object enters ROI");
    
    // Simuleer object in beeld (donker gebied verandert naar licht)
    uint8_t test_frame[240 * 320];
    memset(test_frame, 50, sizeof(test_frame)); // Donkere achtergrond
    
    // Bovenkant van ROI simuleren met object
    for (int y = 10; y < 110; y++) {
        for (int x = 20; x < 160; x++) {
            test_frame[y * 320 + x] = 200; // Wit object
        }
    }
    
    bool detected = detectMotion(test_frame);
    
    if (detected) {
        Serial.println("PASS: Motion correctly detected");
    } else {
        Serial.println("FAIL: Motion not detected");
    }
}

/**
 * Testcase: Geen verandering (globale helderheidswijziging niet triggeren)
 */
void testGlobalBrightnessChange() {
    Serial.println("Test: Global brightness change");
    
    // Alles word donkerder - dit mag NIET triggeren
    uint8_t frame1[240 * 320];
    memset(frame1, 128, sizeof(frame1)); // Middengrijs
    
    uint8_t frame2[240 * 320];
    memset(frame2, 100, sizeof(frame2)); // Donkerder - globale wijziging
    
    bool detected = detectMotion(frame2);
    
    if (!detected) {
        Serial.println("PASS: Global change not triggered");
    } else {
        Serial.println("FAIL: False positive on global change");
    }
}

/**
 * Testcase: Lokale beweging triggeren binnen ROI
 */
void testLocalMotion() {
    Serial.println("Test: Local motion detection");
    
    uint8_t frame[240 * 320];
    memset(frame, 64, sizeof(frame)); // Donkere omgeving
    
    // Bewegende rat in midden van detectiezone
    for (int y = 150; y < 200; y++) {
        for (int x = 180; x < 240; x++) {
            frame[y * 320 + x] = 150; // Lichte rat
        }
    }
    
    bool detected = detectMotion(frame);
    
    if (detected) {
        Serial.println("PASS: Local motion detected");
    } else {
        Serial.println("FAIL: Missed local motion");
    }
}

void setup() {
    Serial.begin(115200);
    testObjectEntersROI();
    testGlobalBrightnessChange();
    testLocalMotion();
    Serial.println("All tests completed");
}

void loop() {
    // Main firmware loop draait apart
}