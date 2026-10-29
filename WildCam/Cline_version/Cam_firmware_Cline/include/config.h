/**
 * config.h - WildCam ESP32-CAM firmware configuratie
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#ifndef CONFIG_H
#define CONFIG_H

/* Transport Layer Configuration */
#define WILDCAM_HTTP_USER_AGENT "WildCam/1.0"
#define WILDCAM_MAX_RETRIES      5
#define WILDCAM_RETRY_BACKOFF_MS 100
#define WILDCAM_CONNECTION_TIMEOUT_SEC 5
#define WILDCAM_UPLOAD_TIMEOUT_SEC 30

/* Camera Settings (OV2640) */
#define WILDCAM_DETECTION_RES    160
#define WILDCAM_CAPTURE_RES      400
#define WILDCAM_PREVIEW_RES      400
#define WILDCAM_JPEG_QUALITY     85
#define WILDCAM_MAX_JPEG_SIZE    1048576

/* ROI Settings */
#define WILDCAM_ROI_ENABLE       true
#define WILDCAM_DETECT_WIDTH     320
#define WILDCAM_DETECT_HEIGHT    240
#define WILDCAM_DETECT_X_OFFSET  80
#define WILDCAM_DETECT_Y_OFFSET  80
#define WILDCAM_CHANGE_THRESHOLD 32
#define WILDCAM_SMOOTHING        4

/* Detection Settings */
#define WILDCAM_DETECTION_MODE    true
#define WILDCAM_EVENT_MODE        false
#define WILDCAM_COOLDOWN_MS       1000
#define WILDCAM_HEARTBEAT_INTERVAL_SEC 60

/* Buffer & PSRAM Settings */
#define WILDCAM_MAX_QUEUE_SIZE    20
#define WILDCAM_BUFFER_TIMEOUT_MS 30000
#define WILDCAM_LOST_FRAME_COUNT  100

/* WiFi Settings */
#define WILDCAM_MAX_STA_RETRY     5
#define WILDCAM_SCAN_FREQUENCY    30
#define WILDCAM_HEARTBEAT_TIMEOUT_SEC 180

/* Serial Logging */
#define WILDCAM_LOG_LEVEL        true
#define WILDCAM_BOOT_HEADER      "WildCam v1.0"
#define WILDCAM_FIRMWARE_VERSION "1.0.0"

/* Web Server Settings */
#define WILDCAM_WEB_PORT         80
#define WILDCAM_WEB_HOSTNAME     "wildcam.local"
#define WILDCAM_WEB_BUFFER_SIZE  4096

/* Event ID Management */
#define WILDCAM_EVENT_ID_LENGTH  16
#define WILDCAM_FRAME_INDEX_START 1

#endif /* CONFIG_H */