/**
 * types.h - Data structures voor WildCam firmware
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "config.h"

/* =========================================================================
   DATA STRUCTURES
   ========================================================================= */

/** Frame metadata struct - voor alle frames (detectie, capturables) */
struct WildCamFrame {
    uint32_t capture_millis;         // Milliseconden tijdstip van capture
    int8_t  boot_id[16];             // Boot ID (hex bytes)
    uint32_t frame_index;            // Index in event-serie
    int rssi_value;                  // RSSI waardenheid van WiFi signal
    uint32_t timestamp_sec;          // Tijd in seconden voor event-ID
};

/** Event struct - voor opgeslagen events */
struct WildCamEvent {
    uint32_t     boot_id;            // Boot ID (van frame)
    uint32_t     frame_index;        // Frame index binnen event
    bool         has_wifi_connection; // WiFi verbinding status
};

/** Upload result - resultaat van uploadpoging */
struct UploadResult {
    int http_status_code;            // HTTP status code (-1 = fout)
    uint32_t elapsed_ms;             // Tijd die verstreken is in ms
    bool success;                    // Of upload geslaagd is
};

/** Trigger state - machine state voor detectie */
enum class DetectionMode {
    DETECTING = 0,                   // Actieve bewegingsdetectie
    CAPTURING_EVENT = 1,             // Frames verzamelen voor event
    COOLDOWN = 2,                    // Cooldown na trigger
};

/** Upload strategie - retry backoff */
struct BackoffConfig {
    uint32_t base_delay_ms;          // Basis delay in ms (begin)
    uint32_t max_delay_ms;           // Maximale delay in ms
    uint8_t  exponent_limit;         // Maximale exponent
};

/** Camera error codes */
enum class CameraErrorCode {
    SUCCESS = 0,
    NO_BUFFER,                       // Geen buffer beschikbaar
    INVALID_FRAME,                   // Ongeldig frame
    BUFFER_OVERFLOW,                 // Buffer overflow
    CAMERA_RESET_REQUIRED,           // Reset vereist
};

/** Backend connectivity status */
enum class BackendStatus {
    CONNECTED = 0,
    DISCONNECTED,                    // Geen verbinding
    LAST_UPLOAD_FAILED,              // Laatste upload mislukt
};

#endif /* TYPES_H */
