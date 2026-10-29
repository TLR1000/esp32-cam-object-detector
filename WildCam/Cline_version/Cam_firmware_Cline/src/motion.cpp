/**
 * motion.cpp - Bewegingsdetectie voor WildCam
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#include <Arduino.h>
#include <cstring>
#include "config.h"
#include "types.h"

uint8_t* g_roi_buffer = NULL;
uint8_t* g_prev_roi_buffer = NULL;
uint8_t* g_smoothed_buffer = NULL;

void motionInit() {
    uint32_t roi_start = WILDCAM_DETECT_X_OFFSET;
    
    g_roi_buffer = new uint8_t[WILDCAM_DETECT_WIDTH * WILDCAM_DETECT_HEIGHT];
    g_prev_roi_buffer = new uint8_t[WILDCAM_DETECT_WIDTH * WILDCAM_DETECT_HEIGHT];
    g_smoothed_buffer = new uint8_t[WILDCAM_DETECT_WIDTH * WILDCAM_DETECT_HEIGHT];
    
    memset(g_roi_buffer, 0, sizeof(g_roi_buffer));
    memset(g_prev_roi_buffer, 0, sizeof(g_prev_roi_buffer));
    memset(g_smoothed_buffer, 0, sizeof(g_smoothed_buffer));
    
    Serial.println("Motion detection initialized");
}

void captureFrameForMotion(uint8_t* frame, uint32_t frame_size) {
    if (!g_roi_buffer || !frame) return;
    memcpy(g_roi_buffer, frame, sizeof(g_roi_buffer));
}

bool detectMotion() {
    if (!g_roi_buffer || !g_smoothed_buffer) return false;
    
    uint32_t change_count = 0;
    uint32_t total_pixels = WILDCAM_DETECT_WIDTH * WILDCAM_DETECT_HEIGHT;
    
    for (uint32_t y = 0; y < WILDCAM_DETECT_HEIGHT; y++) {
        for (uint32_t x = 0; x < WILDCAM_DETECT_WIDTH; x++) {
            uint8_t current_pixel = g_roi_buffer[y * WILDCAM_DETECT_WIDTH + x];
            
            if (g_smoothed_buffer[y * WILDCAM_DETECT_WIDTH + x] == 0) {
                g_smoothed_buffer[y * WILDCAM_DETECT_WIDTH + x] = current_pixel;
            } else {
                uint8_t new_smoothed = (current_pixel + g_smoothed_buffer[y * WILDCAM_DETECT_WIDTH + x]) / WILDCAM_SMOOTHING;
                if (g_smoothed_buffer[y * WILDCAM_DETECT_WIDTH + x] > 0) {
                    g_smoothed_buffer[y * WILDCAM_DETECT_WIDTH + x] = new_smoothed;
                }
            }
            
            uint8_t diff = abs(current_pixel - g_smoothed_buffer[y * WILDCAM_DETECT_WIDTH + x]);
            
            if (diff >= WILDCAM_CHANGE_THRESHOLD) {
                change_count++;
            }
        }
    }
    
    float motion_ratio = (float)change_count / total_pixels;
    
    return motion_ratio > 0.05;
}
