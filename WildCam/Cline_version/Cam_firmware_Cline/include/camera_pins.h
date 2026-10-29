/**
 * camera_pins.h - ESP32-CAM OV2640 pin mapping
 * Cam_firmware_Cline • v1.0 • September 2026
 */

#ifndef CAMERA_PINS_H
#define CAMERA_PINS_H

// AI-Thinker ESP32-CAM board specific pins voor OV2640 camera
// Geen SD-kaart ondersteuning in v1 - GPIO pinnen vrijgehouden voor toekomstige extensies

/* Camera Data Pins */
#define CAM_SDA      25     // I2C SDA
#define CAM_SCL      27     // I2C SCL

/* OV2640 Pin Mapping - AI-Thinker specific */
#define PIN_XCLK          13    // Pixel clock
#define PIN_PWDN          34    // Power down
#define PIN_RESET         35    // Reset pin (active high)
#define PIN_PIXEL_CLOCK   13    // XCLK line
#define PIN_PPD           36    // PCD/PWD
#define PIN_VSYNC         27    // VSYNC
#define PIN_HREF          38    // HREF/HSYN
#define PIN_D7            5     // D7 data line
#define PIN_D6            4    // D6 - also shared with white LED
#define PIN_D5            19   // D5 - also shared with SD1
#define PIN_D4            18   // D4 - also shared with SD0
#define PIN_D3            5    // D3 - overlapping, use carefully
#define PIN_D2            4
#define PIN_D1            19
#define PIN_D0            18

/* Camera Control (GPIO conflicts acknowledged) */
#define CAM_I2C_SDA_PIN   25
#define CAM_I2C_SCL_PIN   27

// Board definities (AI-Thinker ESP32-CAM)
#ifndef BOARD_HAS_PSRAM
#define BOARD_HAS_PSRAM   // We hebben PSRAM

#endif /* CAMERA_PINS_H */
