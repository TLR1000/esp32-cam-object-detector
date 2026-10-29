# ESP32-CAM Object Detector / WildCam

Deze repository bevat de software, het ontwerp en de hardwaredocumentatie waarmee een WildCam gebouwd wordt: een AI-Thinker ESP32-CAM met OV2640-camera die bewegingsdetectie op het apparaat doet, plus een backend (shoebox) voor opslag en latere dierherkenning. De WildCam zelf is het eindresultaat; dit is het bouwproject.

## Structuur

| Map | Inhoud |
| --- | --- |
| [`WildCam/Design/`](WildCam/Design) | Ontwerpdocumenten (Word en tekst): 01 firmware, 02 aansluiting en flashen via CP2102, 03 backend op shoebox. |
| [`WildCam/Cline_version/`](WildCam/Cline_version) | Firmwarevariant gemaakt met Cline: PlatformIO-project (`Cam_firmware_Cline`) en Arduino-variant (`Cam_firmware_Cline_ino`). |
| [`WildCam/Copilot_version/`](WildCam/Copilot_version) | Firmwarevariant gemaakt met Copilot (`Cam_firmware_Copilot.ino`). |
| [`hardware/`](hardware) | Foto's van de ESP32-CAM en de UART-aansluiting en het [aansluitschema](hardware/wiring-diagram.svg) (flashen via USB-voeding of met externe voeding). |
| [`housing/`](housing) | Behuizingsontwerp (STL en 3DM). |

## Hardware en flashen

Het [aansluitschema](hardware/wiring-diagram.svg) toont twee varianten:

- **A. USB-voeding (werkbank):** de CP2102 voedt de ESP32-CAM via zijn +5V-pin. Sluit dan geen externe voeding aan.
- **B. Externe voeding:** 5 V gestabiliseerd, minimaal 1 A, op de 5V- en GND-pin van de camera. De +5V van de CP2102 blijft vrij; GND is gemeenschappelijk. Gebruik dit als de camera tijdens het flashen in brownout gaat.

Flashen: verbind GPIO0 met GND, schakel de voeding in en upload met 115200 baud. Haal daarna de jumper weg en zet de voeding uit en weer aan. In normaal bedrijf is alleen een 5V-voeding nodig en is de CP2102 niet nodig. Details staan in ontwerpdocument 02 in [`WildCam/Design/`](WildCam/Design).

## Wifi-instellingen

Wifi-gegevens en tokens horen in een lokaal `secrets.h` en blijven buiten Git (zie `.gitignore`). Gebruik een `secrets.example.h` als sjabloon waar dat aanwezig is.

## Eerdere inhoud

Een eerdere opzet van deze repository bevatte onder meer `camera-test/`, `BOUWINSTRUCTIE.md` en extra behuizingsbestanden. Die zijn bij de herstructurering verwijderd, maar staan nog in de git-geschiedenis (commit `a5d1063`).
