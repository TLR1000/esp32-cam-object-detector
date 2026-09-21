# ESP32-CAM Object Detector / WildCam

Project voor een AI-Thinker ESP32-CAM met OV2640-camera. Eerste stap: camerabeeld live bekijken via het eigen wifi-netwerk van de ESP32-CAM. Bewegingsdetectie en de backend zijn beschreven in de ontwerpdocumenten en nog niet geïmplementeerd.

## Cameratest

De eerste sketch staat in [`camera-test`](camera-test/README.md), inclusief PlatformIO-configuratie, Arduino-sketch en aansluit-/flashinstructies.

Na flashen: voeding uit, GPIO0-GND-jumper verwijderen en opnieuw inschakelen. Met lokale wifi-instellingen in `camera-test/CameraTest/secrets.h` verbindt de camera met dat netwerk; open **http://wildcam-test.local/** of het IP-adres uit de seriële uitvoer. Deze instellingen blijven buiten Git.

Zonder wifi-instellingen of bij een mislukte verbinding is het test-accesspoint beschikbaar: **WildCam-Test**, wachtwoord **camera-test-32**, pagina **http://192.168.4.1/**. Dit wifi-netwerk biedt geen internet. De bediening van versie 2 werkt zonder JavaScript.

De browser toont MJPEG-livebeeld en kan een losse JPEG openen. Met PSRAM is de resolutie 800 × 600; zonder PSRAM 320 × 240. Gebruik voor de test één kijker tegelijk.

## Status van de verificatie

- PlatformIO-build voor `esp32cam` succesvol met Espressif32 6.12.0 / Arduino ESP32 2.0.17.
- Op 21 september 2026 succesvol geüpload via CP2102 op COM5 naar ESP32-D0WD revision 1.0; de uploader heeft de geschreven data geverifieerd.
- De gebruiker heeft werkend livebeeld via Skynet bevestigd. Detectieprestaties en vertraging zijn nog niet gemeten.

## Ontwerp en behuizing

**Begin voor nieuwe bouwtaken bij [BOUWINSTRUCTIE.md](BOUWINSTRUCTIE.md)**. Versie 1.1 bepaalt de camera-architectuur, prestatieproef, frame- en API-contracten, eventafsluiting en taakverdeling. De firmware- en backendteksten zijn daarop bijgewerkt. De Word-bestanden blijven ongewijzigde historische versies 1.0.

De genummerde documenten staan als Word-bestand en als leesbare tekst in de hoofdmap:

1. ESP32-CAM-firmwareontwerp.
2. Aansluiting en flashen via CP2102.
3. WildCam-backend op shoebox.

De bestaande STL-, 3DM- en G-code-bestanden bevatten het behuizingsontwerp en printmateriaal.

Gedownloade tools en gegenereerde buildbestanden worden via `.gitignore` buiten Git gehouden. Het wifi-wachtwoord hierboven is uitsluitend het vaste testwachtwoord; pas het aan voor een latere installatie.
