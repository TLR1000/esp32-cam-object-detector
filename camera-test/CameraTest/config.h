#pragma once

// Optional local network credentials; never commit secrets.h.
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif
#define CAMERA_HOSTNAME "wildcam-test"
#define AP_SSID "WildCam-Test"
#define AP_PASSWORD "camera-test-32"
