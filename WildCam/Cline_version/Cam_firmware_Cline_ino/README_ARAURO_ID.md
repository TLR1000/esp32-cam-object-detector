# WildCam ESP32-CAM Arduino Sketch

## Quick Start

1. **Open Arduino IDE**  
   ```bash
   # Open Arduino IDE en selecteer board: Tools → Board → "ESP32 by Espressif Systems"
   # Upload speed: 115200 baud
   # Port: COM x (CP2102 USB adapter)
   ```

2. **Kopieer de sketch bestanden**  
   Schep een nieuwe Arduino sketch project met de volgende bestanden:
   - `Cam_firmware_Cline.ino` (hoofdschetch)
   - `Library.ino` (externe bibliotheken)
   - `Settings.ino` (configuratie variabelen)

3. **WiFi configuratie**  
   Bewerk de WiFi credentials in `Settings.ino`:
   ```cpp
   char WIFI_SSID[32] = "your_ssid";
   char WIFI_PASSWORD[64] = "your_password";
   ```

4. **Flashen**  
   Klik op **Upload** in Arduino IDE na configuratie.

## PlatformIO vs Arduino IDE

| Functie          | PlatformIO    | Arduino IDE         |
|-------------------|----------------|---------------------|
| Build             | Automatisch    | Handmatig           |
| Configuratie      | platformio.ini | Settings.ino        |
| Upload            | pio run        | Arduino → Upload    |
| Debugging         | Full support   | Serial Monitor      |

## Bestanden-overzicht

```
Cam_firmware_Cline_ino/
├── Cam_firmware_Cline.ino      # Hoofd sketch met setup() en loop()
├── Library.ino                 # Externe bibliotheken en includes  
├── Settings.ino                # Configuratie variabele en constants
└── .gitignore                  # Git ignore rules voor IDE output
```

## Tips voor Arduino IDE

- Gebruik `File → Preferences` om de board manager URL te wijzigen:
  ```
  https://espressif.github.io/arduino-esp32/package_esp32_dev_index.json
  ```

- Voor debuggen, gebruik **Serial Monitor** (Tools → Serial Monitor) bij 115200 baud.

## Vereisten

- Arduino IDE ≥ 1.8.x
- ESP32 Arduino board package ingezet
- CP2102 USB-UART adapter voor flashen in development mode