/**
 * uploader.cpp - Frame upload naar WildCam Backend API
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#include <Arduino.h>
#include <HTTPClient.h>
#include "config.h"
#include "types.h"

// Backend configuratie
#define WILDCAM_API_URL "/api/v1/frames"
#define WILDCAM_BOOT_ID_LEN 16

// Upload statistics
uint32_t g_frames_uploaded = 0;
uint32_t g_upload_failures = 0;
uint32_t g_last_upload_success_time = 0;

/**
 * Initialisatie van uploader (backend configuratie)
 */
void uploaderInit(const char* backend_url, uint8_t boot_id[16]) {
    Serial.printf("Uploader initialized\n");
    Serial.printf("Boot ID: ");
    for(int i=0; i<16; i++) Serial.print(boot_id[i], HEX);
    Serial.println();
    
    // Store backend URL in NVS/Preferences
}

/**
 * Upload een frame naar de WildCam Backend API
 */
UploadResult uploadFrame(uint8_t* jpeg_data, uint32_t jpeg_size) {
    UploadResult result = {-1, 0, false};
    
    if (!jpeg_data || jpeg_size == 0) {
        Serial.println("Invalid JPEG data for upload");
        return result;
    }
    
    // Generate frame index voor unieke identificatie
    uint32_t frame_index = ++g_last_upload_success_time;
    
    // Build request headers en payload
    HTTPClient http;
    http.setTimeout(WILDCAM_UPLOAD_TIMEOUT_SEC * 1000);
    http.useHTTPAuthentication(false);
    
    if (!http.begin(Serial, WILDCAM_BOOT_ID_HEADER, g_last_upload_success_time)) {
        Serial.println("HTTP client init failed");
        result.http_status_code = -1;
        return result;
    }
    
    // Set headers (X-WildCam-ID, X-Boot-ID, X-Event-ID, etc.)
    http.setHeader(WILDCAM_BOOT_ID_HEADER, frame_index);
    http.setHeader("Content-Type", "image/jpeg");
    http.setHeader("Content-Length", String(jpeg_size).c_str());
    
    // Send JPEG POST request
    int response_code = http.GET("/upload");
    
    if (response_code == 200 || response_code == 201) {
        result.http_status_code = response_code;
        result.success = true;
        g_frames_uploaded++;
        Serial.printf("Upload successful: HTTP %d\n", response_code);
    } else if (response_code >= 400 && response_code < 500) {
        // Client error - geen snelle retry
        Serial.printf("Client error from backend: HTTP %d\n", response_code);
        result.http_status_code = response_code;
    } else if (response_code >= 500) {
        // Server error - exponential backoff
        g_upload_failures++;
        uint32_t elapsed = millis() - result.elapsed_ms;
        
        if (elapsed < WILDCAM_RETRY_BACKOFF_MS * pow(2, min(g_upload_failures, g_error_max_retry_exponent))) {
            // Retry later
            Serial.printf("Upload failed, retrying in %dms\n", elapsed);
        } else {
            result.success = false;
        }
    }
    
    http.end();
    
    return result;
}

/**
 * Generate event ID (boot_id + frame_index)
 */
void generateEventID(uint8_t boot_id[16], uint32_t frame_index, uint32_t& event_id) {
    // Combine boot_id en frame index voor unieke event identifier
    for(int i = 0; i < 16; i++) {
        event_id |= (boot_id[i] << ((i * 8) % 32));
    }
    event_id += frame_index;
}
