# Eerste sketch: camera via eigen wifi

Doel: beeld van de AI-Thinker ESP32-CAM met OV2640 beoordelen. Geen backend of bewegingsdetectie. MJPEG-livebeeld op 800 × 600 met PSRAM; zonder PSRAM 320 × 240. De witte flits-LED blijft uit.

## Gebruik na flashen

1. Schakel de voeding uit, verwijder de GPIO0-GND-jumper en schakel weer in.
2. Verbind telefoon of laptop met **WildCam-Test**, wachtwoord **camera-test-32**.
3. Blijf verbonden wanneer je apparaat meldt dat dit netwerk geen internet heeft.
4. Open **http://192.168.4.1/**. Livebeeld start automatisch. Gebruik één kijker tegelijk.
5. Met **Stop** stop je de stream; **Open foto** opent een JPEG in een nieuw tabblad.

Deze test heeft een vast wifi-wachtwoord, instelbaar in `CameraTest/config.h`. Wie met dit netwerk verbonden is kan het beeld bekijken. Voor een toekomstige installatie het wachtwoord aanpassen.

## Build en flash

Vanuit deze map, met PlatformIO:

```powershell
pio run
pio run -t upload --upload-port COM5
pio device monitor --port COM5 --baud 115200
```

Vervang COM5 door de werkelijk gedetecteerde CP2102-poort. GPIO0 moet bij reset voor het uploaden aan GND liggen. Druk zo nodig op reset terwijl de uploader verbinding zoekt. Na upload GPIO0 losmaken en resetten.

Arduino IDE: open `CameraTest/CameraTest.ino`, gebruik het Espressif ESP32-boardpakket (de PlatformIO-build gebruikt Arduino ESP32 2.0.17), selecteer **AI Thinker ESP32-CAM**, uploadsnelheid 115200.

## Aansluiting CP2102

| CP2102 | ESP32-CAM |
| --- | --- |
| 5V / VBUS | 5V |
| GND | GND |
| TXD, 3,3V-logica | U0R / GPIO3 |
| RXD | U0T / GPIO1 |

Voed niet uit de 3,3V-pin. Bij onvoldoende USB-voeding: stabiele externe 5V-voeding (circa 1 A of meer), gedeelde GND, en CP2102-5V loslaten. Geen SD-kaart nodig.

## Controle op hardware

- Seriële monitor op 115200 toont PSRAM, camerastatus en het webadres.
- Controleer scherpte, kleur, oriëntatie en vloeiendheid gedurende enkele minuten.
- Test Stop, Start livebeeld, Open foto en opnieuw inschakelen.
- Bij camera-initfout: voeding uit en cameraflatkabel/pinout controleren.
- Bij brownout/herstarts: voeding en USB-kabel controleren.
- Geen COM-poort: controleer USB-datakabel en CP2102-driver in Apparaatbeheer.

Referentie voor camera-initialisatie en MJPEG: het officiële Espressif CameraWebServer-voorbeeld, https://github.com/espressif/arduino-esp32/tree/2.0.17/libraries/ESP32/examples/Camera/CameraWebServer .
