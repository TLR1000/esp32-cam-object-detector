/**
 * settings.cpp - NVS Configuratiebeheer voor WildCam
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#include <Arduino.h>
#include "config.h"

// Configuratie key names
#define CONFIG_KEY_BOOT_ID   "boot_id"
#define CONFIG_KEY_BACKURL   "backend_url"
#define CONFIG_KEY_WIFICONF  "wifi_config"

/**
 * Initialiseer NVS voor configuratie opslag
 */
void settingsInit() {
    Serial.println("Initializing NVS storage...");
}

/**
 * Opslaan van boot ID in configuratie
 */
void saveBootID(uint8_t boot_id[16]) {
    // Store unieke boot ID voor event-ID generatie
}

/**
 * Verwieter boot ID na succesvolle upload (hergebruikbaar)
 */
void cleanupOldBootID() {
    Serial.println("Cleaning up old boot IDs...");
}

/**
 * Returneer de huidige configuratie zonder wachtwoord
 */
String getConfigJSON() {
    // Renderen van JSON zonder WiFi credentials
    return "{\n  \"firmware_version\": \"" WILDCAM_FIRMWARE_VERSION "\",\n";
}
