/**
 * camera.cpp - Camera frame handling voor WildCam
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#include <Arduino.h>
#include "config.h"
#include "camera_pins.h"
#include "types.h"

// PSRAM buffer configuratie
#define MAX_CAPTURES_PER_EVENT 3  // Opnemen van 3 frames per event

/* =========================================================================
   FRAME BUFFER MANAGEMENT
   ========================================================================= */

uint8_t* g_frame_buffer = NULL;      // Camera framebuffer (camera.init() allocates)
uint8_t* g_psram_capture[MAX_CAPTURES_PER_EVENT]; // PSRAM buffers voor captures
uint32_t g_captured_frames[MAX_CAPTURES_PER_EVENT];  // Frame indices in PSRAM

/* =========================================================================
   CAMERA OPERATIONS
   ========================================================================= */

/**
 * Capture een frame vanuit camera buffer
 * Retourneert ESP_OK op succes
 */
esp_err_t captureFrame(uint8_t* dest, uint32_t max_bytes) {
    if (dest == NULL || g_frame_buffer == NULL) {
        return ESP_FAIL;
    }
    
    // Beeldbuffer copier naar PSRAM voor upload voorbereiding
    memcpy(dest, g_frame_buffer, min(max_bytes, WILDCAM_MAX_JPEG_SIZE));
    
    return ESP_OK;
}

/**
 * Frame buffer vrijmaken (camera niet blokkeren)
 */
void freeCameraFrame(uint8_t* frame) {
    if (frame == NULL) return;
    // Camera kan direct naar volgende frame gaan
    esp_err_t ret = camera.next();
    if (ret != ESP_OK) {
        Serial.printf("Error getting next frame: %d\n", ret);
    }
}

/**
 * Verwijder frame uit PSRAM buffer na succesvolle upload
 */
void removeFrameFromQueue(uint32_t frame_index) {
    for (uint8_t i = 0; i < MAX_CAPTURES_PER_EVENT; i++) {
        if (g_captured_frames[i] == frame_index) {
            // Buffer vrijmaken voor hergebruik
            delete[] g_psram_capture[i];
            g_psram_capture[i] = NULL;
            g_captured_frames[i] = 0;
            return;
        }
    }
}

/* =========================================================================
   BUFFER MANAGEMENT
   ========================================================================= */

void initBuffers() {
    // Alleen als PSRAM beschikbaar en buffer nodig is
    esp_err_t ret;
    
    if (esp_has_psram()) {
        g_frame_buffer = camera.framebuffer();  // Camera buffer verwerven
        
        // PSRAM buffers voor captures initialiseren
        for (uint8_t i = 0; i < MAX_CAPTURES_PER_EVENT; i++) {
            uint8_t* buf = new uint8_t[WILDCAM_MAX_JPEG_SIZE];
            if (buf) {
                g_psram_capture[i] = buf;
                memset(buf, 0, WILDCAM_MAX_JPEG_SIZE);
            }
        }
    } else {
        Serial.println("WARNING: No PSRAM available!");
    }
}

/**
 * Controleer of buffer vol is (lossen frames bij overflow)
 */
bool isBufferFull() {
    uint32_t count = 0;
    for (uint8_t i = 0; i < MAX_CAPTURES_PER_EVENT; i++) {
        if (g_psram_capture[i] && g_captured_frames[i] > 0) {
            count++;
        }
    }
    return count >= WILDCAM_MAX_QUEUE_SIZE;
}

uint32_t getLostFrameCount() {
    uint32_t lost = 0;
    // Tel verloren frames (buffer overflow events)
    for (uint8_t i = 0; i < MAX_CAPTURES_PER_EVENT; i++) {
        if (g_psram_capture[i] && g_captured_frames[i] > WILDCAM_MAX_QUEUE_SIZE) {
            lost += g_captured_frames[i] - WILDCAM_MAX_QUEUE_SIZE;
        }
    }
    return lost;
}
