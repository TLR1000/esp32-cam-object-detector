# ESP32-CAM pinout (AI-Thinker)

Beschrijving van alle pinnen op de header van de AI-Thinker ESP32-CAM met OV2640. Zie [esp32cam.jpg](esp32cam.jpg) (voorkant) en [esp32cam_back.jpg](esp32cam_back.jpg) (achterkant). Voor de bedrading met een CP2102 zie het [aansluitschema](wiring-diagram.svg).

## Voeding

Voed het bord via de pin met het label **5V**. De AMS1117-3.3 op het bord maakt daar 3,3 V van voor de ESP32, camera en PSRAM. Gebruik een stabiele bron van minimaal circa 1 A (camera en wifi geven stroompieken) en verbind altijd de GND's.

Geef **geen 5 V op de 3V3-pin of op VCC** zolang je niet zeker weet hoe VCC is doorverbonden (zie hieronder). De ESP32 verdraagt maximaal 3,3 V.

## Indeling

Gezien vanaf de voorkant (de kant met de camera), camera bovenaan, zoals op [esp32cam.jpg](esp32cam.jpg):

```
      linker rij              rechter rij
      (van boven naar onder)  (van boven naar onder)

      5V                      3V3
      GND                     IO16
      IO12                    IO0
      IO13                    GND
      IO15                    VCC
      IO14                    U0R
      IO2                     U0T
      IO4                     GND
```

## Linker rij

| Pin | GPIO | Functie en aandachtspunten |
| --- | --- | --- |
| 5V | – | Voedingsingang 5 V. Hier hoort de voeding op. |
| GND | – | Massa. |
| IO12 | GPIO12 | Vrij, maar een **strappingpin** (MTDI): moet bij het opstarten laag zijn. Hoog bij opstarten kan het booten laten mislukken. Deelt de functie met microSD (HS2_DATA2). |
| IO13 | GPIO13 | Vrij. microSD (HS2_DATA3). |
| IO15 | GPIO15 | Vrij, maar een **strappingpin** (MTDO): bepaalt of de boot-log op de seriële poort verschijnt. microSD (HS2_CMD). |
| IO14 | GPIO14 | Vrij. microSD (HS2_CLK). |
| IO2 | GPIO2 | Vrij, maar een **strappingpin**: moet laag of onbeslist zijn bij flashen. microSD (HS2_DATA0). |
| IO4 | GPIO4 | Stuurt de **witte flits-LED** op het bord aan (hoog is aan). Deelt de functie met microSD (HS2_DATA1). Gebruik de pin niet voor iets anders als je de flits wilt houden. |

## Rechter rij

| Pin | GPIO | Functie en aandachtspunten |
| --- | --- | --- |
| 3V3 | – | 3,3 V. Uitgang van de regelaar, of voedingsingang als je het bord direct op 3,3 V voedt (dan niet tegelijk 5V gebruiken). Nooit 5 V erop zetten. |
| IO16 | GPIO16 | Op dit bord verbonden met de chip-select van het PSRAM. Met PSRAM niet als gewone I/O te gebruiken. |
| IO0 | GPIO0 | **Opstartmodus:** laag bij reset of inschakelen = flashmodus (download). Los = normaal opstarten. Na het starten gebruikt de camera de pin als kloksignaal (XCLK), dus niet voor andere doeleinden. |
| GND | – | Massa. |
| VCC | – | Voedingspin die via een soldeerjumper (0 Ω) naar 3,3 V of 5 V is doorverbonden. Naast de pin staan twee posities met de labels `5V` en `3V3`. Welke bezet is, bepaalt wat VCC levert. Controleer dat op je bord voordat je VCC als voeding gebruikt. Gebruik anders de pin met het label 5V. |
| U0R | GPIO3 | Seriële ontvangst (RX). Verbind met **TXD** van de CP2102. |
| U0T | GPIO1 | Seriële zending (TX). Verbind met **RXD** van de CP2102. |
| GND | – | Massa. |

Drie pinnen zijn GND: links bovenaan en twee rechts (naast IO0 en onderaan naast U0T). Ze zijn onderling verbonden.

## Niet uitgevoerd op de header

Deze signalen zitten vast op het bord en zijn niet als pin beschikbaar. Ze staan in [camera_pins.h](../test/camera-webserver/src/camera_pins.h).

| Signaal | GPIO |
| --- | --- |
| Camera D0 tot D7 | 5, 18, 19, 21, 36, 39, 34, 35 |
| Camera XCLK | 0 |
| Camera PCLK | 22 |
| Camera VSYNC | 25 |
| Camera HREF | 23 |
| Camera SCCB SDA (SIOD) | 26 |
| Camera SCCB SCL (SIOC) | 27 |
| Camera PWDN | 32 |
| Rode LED op de achterkant (LED1) | volgens de AI-Thinker-documentatie GPIO33, actief laag. Op sommige borden brandt hij ook bij voeding. Gebruik hem dus niet als bewijs van voeding. |

## Overig op het bord

- **RST-knop** op de achterkant, rechtsboven. Reset het bord zonder de voeding los te halen.
- **Antenne:** een antenne op de printplaat en een IPEX-aansluiting voor een externe antenne. Een 0 Ω-weerstand naast de IPEX-aansluiting bepaalt welke actief is. Zit die op de externe stand zonder antenne, dan is het wifi-bereik zeer klein.
- **microSD-sleuf** aan de voorkant. De sketches in deze repository gebruiken geen SD.
- **PSRAM:** een IPS6404L-chip (volgens de typeaanduiding 64 Mbit, dus 8 MB) is zichtbaar op de achterkant. De seriële monitor bevestigt de grootte bij het opstarten.
