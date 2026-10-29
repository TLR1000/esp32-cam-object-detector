# WildCam ESP32-CAM Firmware (v1.0)

Firmware voor klassieke AI-Thinker-achtige ESP32-CAM-borden met OV2640-camera en PSRAM.

## Doel

Dit project specificeert de firmware voor wildcam1, wildcam2, … Die detecteren lokale objecten die het ingestelde Region of Interest (ROI) binnenkomen, maken JPEG-opnamen en sturen events naar de WildCam-backend op shoebox.

## Acceptatiecriteria

- ✓ wildcam1 en wildcam2 draaien dezelfde binary met verschillende NVS-configuratie
- ✓ Detectie blijft lopen terwijl een HTTP-upload blokkeert of time-out
- ✓ Backend-uitval kan geen onbeperkt RAM gebruiken; verloren frames worden geteld
- ✓ Globale lichtverandering in de ROI triggert normaal niet; lokale beweging wel
- ✓ .local toont preview/livebeeld en zichtbare ROI

## Build & Flashen

### Vereisten

1. **PlatformIO** (https://platform.io) of Arduino IDE met ESP32 board support
2. **ESP32-CAM board**: AI-Thinker ESP32-CAM met OV2640 camera en 4MB PSRAM

### Flashen via CP2102 USB-UART Adapter

```bash
# 1. GPIO0 verbinden met GND voor bootloader mode
# 2. TXD->U0R (GPIO3), RXD->U0T (GPIO1), GND->GND verbinden
# 3. PlatformIO flashen met 115200 baud

pio run -t upload

# 4. Na succes: GPIO0 jumper verwijderen en power-cycle
```

### Normaal bedrijf

Gebruik een gestabiliseerde 5V-voeding van minimaal 1A in het veld:

- 5V voeding --> ESP32-CAM 5V-pin
- GND --> ESP32-CAM GND
- GPIO0 los laten voor normaal booten

## Configuratie

De firmware gebruikt NVS/Preferences voor alle runtime configuratie. Twee hoofdconfiguraties:

```ini
; Wildcam1 - standaard configuratie
WILDCAM_ID=wildcam1
BACKEND_URL=http://shoebox.local/api/v1/frames

; Wildcam2 - aparte instelling, zelfde binary
WILDCAM_ID=wildcam2
```

## Aansluitschema (CP2102)

| CP2102 | ESP32-CAM | Functie                    |
|--------|-----------|----------------------------|
| 5V     | 5V        | Werkbankvoeding            |
| GND    | GND       | Gemeenschappelijke massa   |
| TXD    | GPIO3     | Adapter TX -> ESP32 RX     |
| RXD    | GPIO1     | Adapter RX <- ESP32 TX     |

## Architetuur-overzicht

```
CameraFrames
  ├── DETECTIEMODUS: lage resolutie (160x120)
  │   ├── ROI-blokstatistiek
  │   ├── Vergelijking met vorig frame
  │   └── trigger-state-machine
  │
  └── EVENTMODUS: JPEG-opname
      ├── Kopie naar PSRAM-wachtrij
      └── Cameraframebuffer direct vrijgeven

Upload draait parallel (onafhankelijk van WiFi status)
```

## Tests

```bash
pio test --test-dir=test/test_motion
```

## License

Proprietair voor WildCam project, 2026.
