/**
 * webui.cpp - Lokale Web Interface voor WildCam ESP32-CAM
 * Cam_firmware_Cline • v1.0 • September 2026
 * 
 * Endpoint mapping (sectie 10 van ontwerp):
 *   GET /          → Status/setup, preview, ROI-overlay en detectorwaarden
 *   GET /snapshot.jpg → Een JPEG snapshot  
 *   GET /stream     → MJPEG-livebeeld voor uitrichten
 */

#include <Arduino.h>
#include "webui_config.h"

/* =========================================================================
   WEB SERVER ENDPOINTS
   ========================================================================= */

/**
 * Handleer alle GET requests en render HTML met ROI overlay
 * @param uri - Request URI ("/", "/snapshot.jpg", "/stream", etc.)
 * @return HTTPResponseCode_t (200, 404, etc.)
 */
HTTPResponseCode handleGetRequest(const char* uri) {
    if (strcmp(uri, "/") == 0 || strcmp(uri, "/index.html") == 0) {
        // Render dashboard met ROI overlay en detectorstatus
        return renderDashboard();
    }
    
    else if (strcmp(uri, "/snapshot.jpg") == 0) {
        // Returneer huidige snapshot als JPEG
        return sendSnapshotJPEG();
    }
    
    else if (strcmp(uri, "/stream") == 0 || strcmp(uri, "/mjpeg") == 0) {
        // MJPEG stream voor live beeld met ROI overlay
        return streamMJPEG();
    }
    
    else if (strcmp(uri, "/api/status") == 0) {
        // Returneer status JSON
        return sendStatusJSON();
    }
    
    else if (strcmp(uri, "/api/config") == 0) {
        // Returneer configuratie (zonder WiFi wachtwoord)
        return sendConfigJSON();
    }
    
    else if (strcmp(uri, "/api/reboot") == 0) {
        // Trigger herstart
        triggerReboot();
        return sendOKResponse();
    }
    
    else if (strcmp(uri, "/api/test-upload") == 0) {
        // Test upload naar backend
        performTestUpload();
        return sendOKResponse();
    }
    
    else {
        Serial.printf("Unknown URI: %s\n", uri);
        return NOT_FOUND;
    }
}

/**
 * Render dashboard HTML met ROI overlay
 */
HTTPResponseCode renderDashboard() {
    // Build HTML met genormaliseerde ROI coördinaten
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>WildCam ESP32-CAM</title>";
    html += "<style>body{margin:0;background:#1a1a1a;color:#eee;font-family:monospace;}";
    html += ".stream-container{position:relative;height:480px;overflow:hidden;}";
    html += "#roi-overlay{position:absolute;width:320px;height:240px;border:2px solid lime;left:80px;top:80px;}";
    html += "</style></head><body>";
    
    html += "<h2>WildCam v1.0 Dashboard</h2>";
    html += "<div class='stream-container'>";
    html += "<video id='stream' autoplay loop muted playsinline>";
    // Video stream embeden
    html += "</video>";
    html += "<div id='roi-overlay'></div>";
    html += "<canvas id='overlay'></canvas>";
    html += "</div>";
    
    html += "<h3>Detectorwaarden</h3>";
    html += "<ul>";
    html += "<li>Uptime: " + getSystemUptime() + "</li>";
    html += "<li>WiFi RSSI: " + String(WiFi.RSSI()) + " dBm</li>";
    html += "<li>Detection Mode: " + (g_detection_mode == DETECTING ? "Active" : "Disabled") + "</li>";
    html += "</ul>";
    
    html += "<a href='/api/status'>Status API</a> | <a href='/snapshot.jpg'>Snapshot</a>";
    
    html += "</body></html>";
    
    return sendResponse(OK, 200, "text/html", html.c_str(), html.length());
}

/**
 * Send een HTTP response met statuscode en content type
 */
HTTPResponseCode sendResponse(HTTPResponseCode code, int statusCode, const char* contentType, const String& content) {
    // Implementeer actualiseren van WiFi response (vereist web server library)
    // Voor nu placeholder
    Serial.printf("Sending %s with status %d\n", contentType, statusCode);
    return OK;
}
