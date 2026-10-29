/**
 * capture.cpp - Frame capture en event opname voor WildCam
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#include <Arduino.h>
#include "config.h"
#include "types.h"

// Capture status tracking
uint32_t g_event_frames_captured = 0;
uint32_t g_last_capture_millis = 0;

/**
 * Geef het huidige frame van de camera
 */
esp_err_t getCameraFrame(uint8_t*& dest, uint32_t& size) {
    if (!dest && !size) {
        return ESP_FAIL;
    }
    
    // Camera framebuffer verwerven
    dest = (uint8_t*)camera.framebuffer();
    size = camera.frame_size().height * camera.frame_size().width;
    
    return ESP_OK;
}

/**
 * Capture een frame in JPEG en opslaan in PSRAM buffer
 */
esp_err_t captureJPEG(uint8_t* jpeg_buf, uint32_t& jpeg_size) {
    // Camera settings voor JPEG compressie
    camera.set_pvs(false);
    
    esp_err_t ret = camera.capture(0, 16);
    
    if (ret != ESP_OK) {
        Serial.printf("Camera capture failed: %d\n", ret);
        return ret;
    }
    
    // Copier naar PSRAM buffer voor upload
    jpeg_size = camera.framebuf_len();
    memcpy(jpeg_buf, camera.framebuffer(), jpeg_size);
    
    // Framebuffer vrijgeven direct
    camera.free_frame(0);
    
    return ESP_OK;
}

/**
 * Initialisatie van event capture structuur
 */
void captureInit() {
    g_event_frames_captured = 0;
    g_last_capture_millis = millis();
    
    Serial.println("Capture initialized");
}

/**
 * Voer een capture sequence uit voor event (meerdere frames)
 */
uint32_t captureEventFrames(uint8_t** jpeg_buffers, uint32_t& total_size, uint32_t max_frames) {
    if (!jpeg_buffers || !total_size) return 0;
    
    Serial.printf("Starting event capture sequence...\n");
    g_event_frames_captured = 0;
    
    // Maximum frames per event (geeft kwaliteit vs bandbreedte balans)
    uint32_t frames_to_capture = min(max_frames, 3);
    uint32_t total_captured = 0;
    
    for (uint32_t i = 0; i < frames_to_capture; i++) {
        uint8_t* current_frame = NULL;
        uint32_t frame_size = 0;
        
        getCameraFrame(current_frame, frame_size);
        
        if (!current_frame || frame_size == 0) {
            Serial.println("Failed to capture frame");
            break;
        }
        
        // Converteer naar JPEG (camera doet dit automatisch met PVS=off)
        uint32_t jpeg_buf = current_frame;
        
        // Kopieer naar PSRAM buffer voor event serie
        if (total_captured < max_frames && jpeg_buffers[total_captured]) {
            memcpy(jpeg_buffers[total_captured], current_frame, min(frame_size, WILDCAM_MAX_JPEG_SIZE));
            
            uint32_t copy_len = frame_size;
            if (copy_len > WILDCAM_MAX_JPEG_SIZE) {
                copy_len = WILDCAM_MAX_JPEG_SIZE;
            }
            
            jpeg_buffers[total_captured] = new uint8_t[copy_len];
            memcpy(jpeg_buffers[total_captured], current_frame, copy_len);
        }
        
        total_size += frame_size;
        g_event_frames_captured++;
        total_captured++;
        
        Serial.printf("Frame captured: %u bytes (%u total)\n", frame_size, total_size);
    }
    
    // Vrijmaken van camera buffers na capture sequence
    for (uint32_t i = 0; i < total_captured && i < max_frames; i++) {
        if (jpeg_buffers[i] && jpeg_buffers[i][0]) {
            delete[] jpeg_buffers[i];
            jpeg_buffers[i] = NULL;
        }
    }
    
    return g_event_frames_captured;
}

/**
 * Returneer het aantal frames gevangen tijdens een event
 */
uint32_t getEventFramesCaptured() {
    return g_event_frames_captured;
}

/**
 * Reset de capture counters
 */
void resetCaptureCounters() {
    g_event_frames_captured = 0;
    g_last_capture_millis = millis();
}
