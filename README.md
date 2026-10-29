# ESP32-CAM Object Detector / WildCam

Project voor een AI-Thinker ESP32-CAM met OV2640-camera: bewegingsdetectie op het apparaat en een backend (shoebox) voor opslag en latere dierherkenning.

## Structuur

| Map | Inhoud |
| --- | --- |
| [`WildCam/Design/`](WildCam/Design) | Ontwerpdocumenten (Word en tekst): 01 firmware, 02 aansluiting en flashen via CP2102, 03 backend op shoebox. |
| [`WildCam/Cline_version/`](WildCam/Cline_version) | Firmwarevariant gemaakt met Cline: PlatformIO-project (`Cam_firmware_Cline`) en Arduino-variant (`Cam_firmware_Cline_ino`). |
| [`WildCam/Copilot_version/`](WildCam/Copilot_version) | Firmwarevariant gemaakt met Copilot (`Cam_firmware_Copilot.ino`). |
| [`hardware/`](hardware) | Foto's van de ESP32-CAM en de UART-aansluiting en het [aansluitschema](hardware/wiring-diagram.svg) (flashen via USB-voeding of met externe voeding). |
| [`housing/`](housing) | Behuizingsontwerp (STL en 3DM). |

## Wifi-instellingen

Wifi-gegevens en tokens horen in een lokaal `secrets.h` en blijven buiten Git (zie `.gitignore`). Gebruik een `secrets.example.h` als sjabloon waar dat aanwezig is.

## Eerdere inhoud

Een eerdere opzet van deze repository bevatte onder meer `camera-test/`, `BOUWINSTRUCTIE.md` en extra behuizingsbestanden. Die zijn bij de herstructurering verwijderd, maar staan nog in de git-geschiedenis (commit `a5d1063`).
