# Camera-webserver

Eenvoudige testsketch voor de AI-Thinker ESP32-CAM met OV2640. De camera start als wifi-accesspoint en toont het livebeeld op een webpagina, met de witte flits-LED aan of uit te zetten. Geen router, backend of bewegingsdetectie nodig.

## Gebruik

1. Flash de sketch (zie hieronder). Haal daarna de GPIO0-GND-jumper weg en zet de voeding uit en weer aan.
2. Verbind telefoon of laptop met wifi **ESP32CAM-Test**, wachtwoord **camera-test-32**.
3. Blijf verbonden als je apparaat meldt dat dit netwerk geen internet heeft.
4. Open **http://192.168.4.1/**. Het livebeeld start automatisch.
5. Zet de flits aan of uit met de links **aan** en **uit** onder de titel. De flits staat bij het opstarten uit.

Gebruik één kijker tegelijk. Wie met het accesspoint verbonden is kan het beeld bekijken. Het wachtwoord staat vast in [`src/main.cpp`](src/main.cpp); pas het aan voor gebruik buiten de werkbank.

## Hoe het werkt

| Poort | Pad | Functie |
| --- | --- | --- |
| 80 | `/` | Webpagina met livebeeld en flitsbediening. `/?flash=1` zet de flits aan, `/?flash=0` uit. |
| 81 | `/stream` | MJPEG-stream. Een eigen server, zodat de pagina bedienbaar blijft terwijl de stream loopt. |

Met PSRAM is de resolutie 800 × 600 (SVGA), zonder PSRAM 320 × 240. De flits zit op GPIO4. De pagina werkt zonder JavaScript.

## Build en flash

Vanuit deze map, met PlatformIO:

```powershell
pio run
pio run -t upload --upload-port COM5
pio device monitor --port COM5 --baud 115200
```

Vervang COM5 door de poort van je CP2102. GPIO0 moet bij het inschakelen aan GND liggen; druk zo nodig op reset terwijl de uploader verbinding zoekt. Bij een mislukte upload door brownout: gebruik een externe voeding. Zie het [aansluitschema](../../hardware/wiring-diagram.svg) voor beide voedingsvarianten.

Arduino IDE: gebruik [`src/main.cpp`](src/main.cpp) als sketch, met het Espressif ESP32-boardpakket (de PlatformIO-build gebruikt Arduino ESP32 2.0.17) en board **AI Thinker ESP32-CAM**.

## Problemen

- **Geen COM-poort:** controleer de USB-datakabel en de CP2102-driver in Apparaatbeheer.
- **"camera starten mislukt" in de seriële monitor:** voeding uit en de camerakabel en pinout controleren.
- **Herhaalde resets (brownout):** voeding of USB-kabel te zwak, gebruik een externe 5V-voeding van minimaal 1 A.
- **Geen beeld maar wel de pagina:** controleer of poort 81 bereikbaar is, bijvoorbeeld via **http://192.168.4.1:81/stream**.
