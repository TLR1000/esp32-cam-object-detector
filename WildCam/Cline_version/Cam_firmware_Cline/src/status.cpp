/**
 * status.cpp - Status tracking voor WildCam ESP32-CAM
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#include <Arduino.h>
#include "config.h"

// Status counters
uint32_t g_detections = 0;
uint32_t g_events = 0;
uint32_t g_frames_captured = 0;
uint32_t g_frames_uploaded = 0;
uint32_t g_dropped_frames = 0;
uint32_t g_camera_failures = 0;

/**
 * Returneer system status JSON voor API endpoint
 */
void getStatus(JSON& root) {
    // System status informatie verzamelen
}

/**
 * Update uptime counter
 */
void updateUptime() {
    // Bereken uptime in seconden
}

/**
 * Incrementeer de detection counter
 */
void incrementDetections() {
    g_detections++;
    Serial.printf("Detection #%u\n", g_detections);
}
