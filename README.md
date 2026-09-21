# ESP32-CAM Object Detector / WildCam

Project voor een AI-Thinker ESP32-CAM met OV2640-camera. Eerste stap: camerabeeld live bekijken via het eigen wifi-netwerk van de ESP32-CAM. Bewegingsdetectie en de backend zijn beschreven in de ontwerpdocumenten en nog niet geïmplementeerd.

## Cameratest

De eerste sketch staat in [`camera-test`](camera-test/README.md), inclusief PlatformIO-configuratie, Arduino-sketch en aansluit-/flashinstructies.

Na flashen: voeding uit, GPIO0-GND-jumper verwijderen en opnieuw inschakelen. Verbind met **WildCam-Test**, wachtwoord **camera-test-32**, en open **http://192.168.4.1/**. Dit wifi-netwerk biedt geen internet.

De browser toont MJPEG-livebeeld en kan een losse JPEG openen. Met PSRAM is de resolutie 800 × 600; zonder PSRAM 320 × 240. Gebruik voor de test één kijker tegelijk.

## Status van de verificatie

- PlatformIO-build voor `esp32cam` succesvol met Espressif32 6.12.0 / Arduino ESP32 2.0.17.
- Op 21 september 2026 succesvol geüpload via CP2102 op COM5 naar ESP32-D0WD revision 1.0; de uploader heeft de geschreven data geverifieerd.
- De camera-initialisatie en het livebeeld moeten nog op de hardware worden gecontroleerd.

## Ontwerp en behuizing

De genummerde documenten staan als Word-bestand en als leesbare tekst in de hoofdmap:

1. ESP32-CAM-firmwareontwerp.
2. Aansluiting en flashen via CP2102.
3. WildCam-backend op shoebox.

De bestaande STL-, 3DM- en G-code-bestanden bevatten het behuizingsontwerp en printmateriaal.

Gedownloade tools en gegenereerde buildbestanden worden via `.gitignore` buiten Git gehouden. Het wifi-wachtwoord hierboven is uitsluitend het vaste testwachtwoord; pas het aan voor een latere installatie.
