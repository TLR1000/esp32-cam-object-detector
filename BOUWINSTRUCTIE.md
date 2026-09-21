# WildCam bouwinstructie

Versie 1.1 — 21 september 2026

Deze specificatie bepaalt de interfaces, taakverdeling en acceptatie voor de parallelle bouw van WildCam. Lees daarnaast document 01 voor firmware, 02 voor bedrading en 03 voor de backend. Dit document is leidend voor gedeelde contracten. De ongewijzigde Word-documenten zijn historische versies 1.0 en geen actuele bouwinstructies.

## Bewezen uitgangspunt en scope

De AI-Thinker ESP32-CAM met OV2640 kan JPEG-beelden leveren en livebeeld via bestaande wifi tonen. De gebruiker heeft dit bevestigd met `camera-test`. De haalbare detectiesnelheid, beschikbare PSRAM, vertraging en betrouwbaarheid bij weinig licht zijn nog niet gemeten. De testsketch is een apart diagnoseprogramma en geen basis om netwerkcode in de detectorlus over te nemen.

V1 detecteert lokale beweging binnen een ROI en bewaart events met JPEG-beelden. Dierherkenning gebeurt later op de server. Hardware gebruikt 2,4GHz-wifi, een CP2102 voor flashen en voorlopig geen SD-kaart. Netwerkgegevens en tokens blijven buiten Git. De bestaande behuizingsbestanden blijven behouden.

## Volgorde en eigenaarschap

1. Leg de onderstaande contracten vast in `contracts/`: C++-headers, JSON Schema/OpenAPI en gedeelde geldige en ongeldige testfixtures. Er is één integratieverantwoordelijke voor contractwijzigingen. Wijzigingen aan een contract vereisen gelijktijdige aanpassing van producent, consument en fixtures.
2. Camera/prestatiemetingen, detector, backend en webinterface mogen daarna onafhankelijk worden gebouwd. Alleen de camera-implementatie en definitieve configuratie wachten op de hardwaremeting; de andere taken gebruiken fixtures en mocks.
3. Integreer eerst één camera naar één backend, daarna meerdere camera's, verstoringen en de fysieke detectieproef. Alleen de camera/integratietaak flasht het gedeelde apparaat. Andere taken flashen niet zelfstandig.

| Taak | Bestanden en verantwoordelijkheid | Afhankelijkheid en resultaat |
| --- | --- | --- |
| Camera en firmwareintegratie | `wildcam-firmware/`, behalve de detectormodule; centrale camera-eigenaar, geheugen, netwerk, uploader, eventbesturing en metingen | Implementeert framecontract en protocol; levert meetrapport en flashbare firmware |
| Bewegingsdetectie | `wildcam-firmware/src/motion.*`, bijbehorende hosttests en fixtures | Verwerkt uitsluitend GrayFrameView; levert scores en bewegingsbesluit zonder hardware- of netwerktoegang |
| Backend | `wildcam-backend/`, behalve templates/static | Implementeert contracten voor ingestie, events, opslag, status en beheer |
| Webinterface | Backendtemplates/static en UI-tests | Gebruikt vastgelegde backendresponses; implementeert geen eigen opslag- of ingestielogica |

De firmwareintegratietaak beheert ook de lokale camerawebpagina. Zo wijzigen backend-UI en firmwaretaken niet dezelfde bestanden. Gebruik per taak een eigen branch/worktree; gedeelde contracten worden eerst gepubliceerd. De gebruiker start de taken afzonderlijk.

## Framecontract

Alle camera-drivercalls en wijzigingen aan sensor/resolutie lopen via één camera-eigenaar. Consumers krijgen nooit een `camera_fb_t*`. Opname, analyse en netwerk hebben begrensde wachtrijen en een expliciet maximumaantal buffers. Alleen analyse heeft voorrang; livebeeld is optioneel en mag frames overslaan.

`GrayFrameView` bevat `sequence` (uint64), `capture_us` (uint64, monotone microseconden sinds boot bij begin van opname), `width`, `height`, `stride` en `pixels` (8-bit luminantie). De pixels zijn alleen geldig gedurende `MotionDetector.process(frame, config)`. De detector bewaart geen pointer en doet geen allocaties per frame, I/O, sleeps of camera-drivercalls. De camera-eigenaar of adapter maakt de luminantiegegevens; JPEG-decodering hoort niet bij de detector.

`MotionResult` bevat dezelfde sequence/capture_us, een bewegingsscore, globale lichtverandering, het aantal aaneengesloten veranderde blokken en `candidate`. De eventcontroller beslist met een tijdvenster over bevestiging en eventgrenzen. ROI is x/y/w/h in 0..1000, na toepassing van vaste beeldoriëntatie; valideer w/h > 0 en x+w/y+h <= 1000. Normaliseer alle resoluties naar hetzelfde zichtveld. Wisseling van oriëntatie, ROI of resolutie reset het vergelijkingsmodel en wordt als meetonderbreking geregistreerd.

Een JPEG voor netwerk of eventopslag is een eigen kopie in een begrensde bufferpool. De camerabuffer wordt teruggegeven voordat netwerkverzending begint. Bezit gaat bij enqueue naar de consumer; die geeft de kopie vrij na aflevering of definitieve verwijdering. Dubbele vrijgave, vasthouden van een driverbuffer door een consumer en onbeperkte allocaties zijn verboden.

Livebeeld heeft maximaal één wachtend recent frame per kijker en maximaal één kijker in v1. Een nieuw frame vervangt een nog niet verzonden frame; een reeds verzonden frame wordt niet gemuteerd. Een langzaam netwerk mag geen FIFO met steeds ouder livebeeld opbouwen. Begrens ook de actieve verzendbuffer. Preview en uploader krijgen geen voorrang op detectie.

## Hardwareproef en geheugenbudget

Vergelijk minimaal 320x240, 640x480 en 800x600 JPEG. Meet per profiel zonder kijker, met één kijker en met vertraagde/offline backend. Leg ook lichtniveau, cameraoriëntatie, voeding, RSSI, firmwareversie en PSRAM vast. Onderzoek twee routes: JPEG opnemen en verkleinen naar luminantie, of expliciete sensor/resolutiewissels. Kies pas daarna de productieroute; meet bij wisselen de duur, onbruikbare overgangsframes en detectieonderbreking.

Rapporteer werkelijk ontvangen detectieframes/s, maximale tijd tussen geanalyseerde opnamen, frameleeftijd bij analyse en verzenden, analyse- en verzendtijden (p50/p95/max), JPEG-grootte, vrije/minimale PSRAM en grootste vrije blok. Bepaal frametijden uit opname-timestamps, niet alleen uit hoe snel `fb_get` terugkomt. Browserweergavevertraging vereist een afzonderlijke visuele proef met een zichtbare klok of beweging.

Budgetteer driverbuffers, decode-/analysewerkruimte, pretriggerbuffer, uploadwachtrij, actieve netwerkbuffers en veiligheidsmarge samen. De eerdere vaste uploadreservering van 2 MiB vervalt. De som moet onder het werkelijk gemeten beschikbare geheugen blijven, ook bij piekframes. Geen heapallocatie bij ieder detectieframe. Bij ontbrekende PSRAM: expliciete degraded-status en apart gevalideerd profiel; geen stilzwijgende volledige prestatieclaim.

## Detectie en events

De voorlopige acceptatiedoelstelling is ten minste 95 detecties uit 100 passages waarbij een afgesproken testobject minimaal 300 ms in de ROI zichtbaar is. Dit is een te toetsen doel, geen bewezen eigenschap. Leg vóór de proef objectgrootte, afstand, snelheid, ROI, lichtcondities en grondwaarheid vast. Meet detectiekans en tijd van eerste zichtbaarheid tot trigger. Rapporteer snelle/korte passages apart. Een niet gehaald doel is een blokkade voor de prestatieclaim, geen reden om de grens stilzwijgend te verlagen.

Start onderzoek rond 10 detectieframes/s en een maximale analyse-onderbreking van 100 ms; dit zijn startdoelen, geen hardwaregaranties. Kies definitieve frequentie en tijdbevestiging op basis van de proef. De oude harde instelling 5 fps en 2 van 3 frames is vervangen door configureerbare tijdbevestiging die ook onregelmatige intervallen controleert. Een groot framegat mag niet als aaneengesloten bewijs tellen. Rapporteer daarnaast valse events in een vaste lege scène en bij globale lichtwisselingen; leg de acceptatiegrens vast vóór de eindproef.

Bewaar beoogd 0,5–1 seconde JPEG-voorgeschiedenis in een ringbuffer, begrensd op bytes en frames. Bij bevestiging blijven de beschikbare pretriggerframes behouden voor het event. Leg werkelijke pretriggerdekking vast; een kleinere dekking bij geheugendruk wordt zichtbaar gemeld. Pretriggerbeelden mogen een lagere resolutie hebben dan bewijsfoto's. Bufferbeheer mag de detectie niet laten wachten.

Beweging verlengt een event. Startwaarden: posttrigger 3 s, bewijsfoto's 3 fps en maximale eventduur 30 s. Bij aanhoudende beweging volgt na 30 s een nieuw event met `previous_event_id`; detectie blijft doorgaan. Er is geen blinde cooldown. Frame-indexen zijn vanaf 0 per event in opnamevolgorde; elke voor een event geselecteerde opname krijgt een index, ook als deze later verloren gaat. Een niet gemaakte geplande opname telt als `capture_failure`, niet als verzonnen frame. Sorteer pretriggerframes vóór triggerframes. Tijdgrenzen zijn configureerbaar en worden in fixtures getest.

## Gemeenschappelijk netwerkcontract versie 1

De nog te bouwen productie-API gebruikt `/api/v1`. `protocol_version` is 1; firmwareversie staat daar los van. Timestamps zijn UTC RFC3339 wanneer gesynchroniseerd, anders null/afwezig. `capture_ms` en `capture_us` zijn monotone tijden sinds boot, geen epoch. Ontvangsttijd wordt altijd door de backend vastgelegd. `clock_synced` hoort per frame bij het moment van opname.

Frame-identiteit: `(camera_id, boot_id, event_id, frame_index)`. Camera-ID voldoet aan `^wildcam[1-9][0-9]*$`. `boot_id` is 32 hextekens met 128 willekeurige bits per boot; event_id is een oplopende teller binnen die boot. Deze afspraak vervangt de oude 8-hex boot-ID en persistente eventteller. JSON encodeert event_id, previous_event_id en uint64-tijden als decimale strings om precisieverlies in JavaScript te voorkomen. Frame-indexen en aantallen blijven begrensde JSON-integers. Voorbeelden gebruiken `7fa31c09000000000000000000000001` als boot-ID.

`POST /api/v1/frames` ontvangt image/jpeg met headers X-WildCam-ID, X-Boot-ID, X-Event-ID, X-Frame-Index, X-Capture-Millis, X-Clock-Synced (true/false), X-RSSI, X-Firmware-Version en X-Protocol-Version (1). X-Captured-At is verplicht bij clock_synced=true en ontbreekt anders. X-Frame-Phase is pretrigger of event. De backend valideert/decodeert JPEG binnen byte- en pixelgrenzen. SHA256 wordt door de backend berekend. Identieke retry betekent dezelfde JPEG en dezelfde onveranderlijke opnamemetadata; RSSI en firmwareversie zijn die bij opname en blijven gelijk tijdens retries.

201 = duurzaam nieuw frame; 200 = exacte retry; 409 = dezelfde identiteit met andere inhoud of opnamemetadata. De bestaande responses met ok/stored/frame_id blijven behouden; frame_id is een decimale string. 400/413/415/422 zijn permanente requestfouten. 401/403 pauzeren upload voor die camera en melden een configuratiefout. Netwerkfouten, 408, 429 en 5xx krijgen begrensde backoff met jitter; respecteer Retry-After binnen de ingestelde bovengrens. Andere 4xx blokkeren de wachtrij niet: tel en registreer het afgewezen item en ga verder. Geen verwijdering bij een onbegrepen of ongeldige succesresponse.

`POST /api/v1/events/close` ontvangt een idempotente definitieve afsluiting: camera_id, boot_id, event_id, previous_event_id (string of null), first_capture_ms, last_capture_ms, trigger_ms, selected_frames, uploaded_frames, dropped_frames, capture_failures, pretrigger_frames, pretrigger_coverage_ms en reason (quiet/max_duration/manual). Tijden zijn decimale strings; coverage is een begrensd integer in ms. Afsluiten gebeurt nadat alle lokale frame-items voor dit event zijn bevestigd of definitief afgevoerd; dan geldt selected_frames = uploaded_frames + dropped_frames. Reserveer een aparte begrensde wachtrij voor afsluitberichten. Ook een event met nul geaccepteerde frames mag worden afgesloten.

Een afsluiting mag de backend bereiken voordat alle frametransacties zichtbaar zijn; de backend accepteert beide volgorden. Exact herhalen geeft 200, eerste afsluiting 201, afwijkende afsluiting 409. Ontvangen frames zijn de backendwaarheid. `capture_closed` betekent dat de camera klaar is; `delivery_complete` betekent dat het aantal duurzaam ontvangen frames gelijk is aan uploaded_frames. `has_loss` volgt uit dropped_frames/capture_failures. Inconsistenties worden zichtbaar en overschrijven geen eerdere gegevens. Een inactiviteitstime-out markeert uitsluitend stale/incomplete, nooit bewezen compleet. Na stroomverlies blijven niet-afgesloten events incomplete; PSRAM biedt geen duurzaamheid.

Heartbeat elke 60 s; backend stale na 180 s. JSON bevat naast bestaande tellers protocol_version, clock_synced, camera_ready, psram_available, capture_fps, detect_fps, max_detect_gap_ms, analysis_p95_ms, frame_age_p95_ms, send_p95_ms, jpeg_bytes_mean, psram_free_bytes, psram_min_free_bytes, queue_frames/bytes en dropped_by_reason. Dropreasons zijn pretrigger_evicted, preview_replaced, upload_queue_full, permanent_reject en age_expired. Normale ringbufferrotatie en previewvervanging tellen niet als verloren eventframes. Tellers resetten per boot. Percentielen gebruiken het laatste volledige venster van 60 s; zonder samples is de waarde null.

## Gezamenlijke acceptatie

- De detector werkt op vaste opnamereeksen zonder camera of netwerk en reproduceert resultaten met dezelfde configuratie.
- De fysieke passageproef en lichtwisselproef zijn uitgevoerd; de rapportage scheidt behaalde waarden van doelen.
- Detectie voldoet ook met één kijker en tijdens tien minuten backenduitval aan de vastgestelde timinggrens. Een volle uploadwachtrij veroorzaakt getelde verliezen, geen vastloper of groeiend geheugenverbruik.
- Eventverlenging, maximale duur, pretriggerdekking, netwerkherstel, reboot en ontbrekende klok zijn getest.
- Backendtests gebruiken dezelfde contractfixtures voor exacte retries, 409, afsluiting vóór/na frames, nul frames, twee camera's en crashherstel van bestand plus database.
- Geen wifi-wachtwoorden, tokens of binaries met daarin credentials in Git. Geen afhankelijkheid van CP2102 na deployment.
- Elk onderdeel levert installatie-/buildinstructies, relevante tests en openstaande beperkingen. Alleen integratie mag de volledige keten als geslaagd markeren.
