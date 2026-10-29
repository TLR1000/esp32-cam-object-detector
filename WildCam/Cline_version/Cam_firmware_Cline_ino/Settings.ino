/**
 * Settings.ino - Configuratie variabele en constanten
 */

// WiFi configuratie (vervul in setup())
char WIFI_SSID[32] = "your_ssid";        // Vervang met jouw SSID  
char WIFI_PASSWORD[64] = "your_password";// Vervang met wachtwoord

// Backend API URL (voor uploads)
const char* BACKEND_URL = "http://shoebox.local/api/v1/frames";
const char BOOT_ID[] = "wildcam001";     // Verander voor wildcam2, etc.

// Camera instellingen
#define CAMERA_WIDTH 320                   // Resolutie breedte
#define CAMERA_HEIGHT 240                  // Resolutie hoogte  
#define DETECT_X_OFFSET 10                 // ROI begin x
#define DETECT_Y_OFFSET 80                 // ROI begin y
#define DETECT_WIDTH 230                   // ROI breedte
#define DETECT_HEIGHT 220                  // ROI hoogte

// Motion detection thresholds  
const int MOTION_CHANGE_THRESHOLD = 50;   // Min verandering per pixel
const int MOTION_SMOOTHING_FACTOR = 4;    // Lerende average factor
const float MOTION_RATIO_THRESHOLD = 0.05; // 5% pixels moeten veranderen

// Upload configuratie  
const int UPLOAD_TIMEOUT_SECONDS = 30;    // Timeout voor API calls  
const int RETRY_BACKOFF_MS = 1000;        // Retry backoff tussen fails
const int MAX_RETRY_EXPONENT = 4;         // Maximale exponent voor backoff

// Frame buffering  
const uint32_t MAX_JPEG_SIZE = 512 * 1024;    // Maximale JPEG buffer grootte (512KB)