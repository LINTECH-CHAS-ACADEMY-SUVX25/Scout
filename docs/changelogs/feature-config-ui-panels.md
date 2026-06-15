# feature/config-ui-panels

CONFIG-knapp i topbaren som öppnar två live-konfigurationspaneler — en för
kameran (CAM CFG, övre vänster) och en för miljösensorn (BME280 CFG, nedre
höger). Panelerna håller samma premium-stil som resten av UI:t och är redan
placerade för att ta emot riktig config-logik när den kopplas in.

## Tillagt

- **CONFIG-knapp i topbaren** — ny klickbar label `CONFIG` till vänster om
  `THEMES`, med `|`-avdelare emellan. Topbaren läser nu
  `CONFIG | THEMES | WiFi` från höger
- **CAM CFG-panel** (övre vänstra hörnet, 240×236 px) — öppnas och stängs
  via CONFIG-knappen. Innehåller tre slider-rader som speglar OV2640:ns
  live-tunbara parametrar via `esp_camera` sensor-API:t:
  - `QUALITY` 0–63 (default 20, matchar `jpeg_quality` i `camera.c`)
  - `BRIGHT` −2..+2 (sensor `set_brightness`)
  - `CONTRAST` −2..+2 (sensor `set_contrast`)
- **BME280 CFG-panel** (nedre högra hörnet, 240×236 px) — öppnas
  tillsammans med CAM CFG men stängs separat. Tre slider-rader som matchar
  konfigurerbara parametrar i `telemetry.c` och `bme280.c`:
  - `INTERVAL` 1–10 s (default 2, matchar `DIAG_INTERVAL_MS = 2000`)
  - `OSAMPLE` x1–x16 via index 1–5 (oversampling, label visar `X1`..`X16`)
  - `FILTER` off/2/4/8/16 via index 0–4 (IIR-koefficient för tryckmätning)
- **Slider-design** — 4 px track (pill-radius, `COL_LINE`), accent-gradient
  indikator (deep→accent vänster→höger), 8 px knob med 1 px
  `COL_PANEL`-ring för "floating dot"-känsla. Värdelabeln till höger
  uppdateras live vid drag utan fördröjning
- **APPLY per panel** — varje panels APPLY-knapp stänger bara sin egen
  panel (inte den andra). `s_config_open` nollställs automatiskt när båda
  är dolda
- **`make_slider_row`** — byggare för key-label + värde-label + slider med
  temafärger. Tar en `lv_event_cb_t` för formateringen så samma byggare
  används för alla rader oavsett enhet (heltal, `N S`, `XN`, `OFF/2/4/8/16`)
- **`make_cfg_sep`** — tunn `COL_LINE`-avdelare (1 px) mellan slider-raderna
  inom en panel, i stil med telemetrikortets radavdelare
- Fyra slider-callbacks: `slider_num_event`, `slider_interval_event`,
  `slider_osample_event`, `slider_filter_event`

## Verifierat

- Simulatorn (`sim/`) bygger rent utan varningar efter varje steg
- Temabyte (SONAR/DESERT/NIGHT OPS) återställer panelernas synlighetsläge
  korrekt via `s_config_open`

## Kvarstår / kopplingspunkter mot `fix/final-refactor`

Panelerna är avsiktligt UI-only — ingen config skickas till hårdvaran än.
Nedan är vad som behöver kopplas in när `fix/final-refactor` mergas:

### Merge-konflikt att räkna med

`fix/final-refactor` ändrar `scout_ui.c` på tre ställen:

1. Tar bort `#include "cam_diag_fmt.h"` och lägger till `#include <stdio.h>`
2. Lägger till `fmt_temp`, `fmt_humi`, `fmt_pres` som statiska funktioner
3. Byter `cam_diag_fmt_*` → `fmt_*` i `s_tele[]`

Dessa ändringar är oöverlappande med config-panelernas kod och löser sig
med en enkel `git mergetool` — behåll båda sidornas tillägg.

### Config-logik som saknas

När protokollet är klart kopplas logiken in i `panel_close_event` precis
innan `lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN)`:

**CAM CFG:** läs slider-värden och skicka via UDP/TCP till `scout_cam`.
Kräver ett nytt `cam_cfg_pkt_t` i `rc_protocol.h` (quality + brightness +
contrast) och en ny port eller ett nytt kommandotypsfält i befintlig
CMD-kanal.

**BME280 CFG:** läs slider-värden och antingen (a) uppdatera
`DIAG_INTERVAL_MS` via en delad `atomic`-variabel som `telemetry_run`
läser, eller (b) skicka ett config-kommando till cam-sidan som sätter om
BME280:s oversampling och filter via `reg_write` i `bme280.c`. Alternativ
(a) för intervallet är enklast; oversampling/filter kräver ett protokolltillägg.

### Slider-handtag

`make_slider_row` returnerar inget i dag. Om man vill läsa värdena utan att
ändra signaturen kan man `lv_obj_get_child` på panelen och caita, men det
enklare sättet är att låta `make_slider_row` returnera `lv_obj_t *` (slidern)
och lagra handtagen i statiska variabler vid sidan av de befintliga
`s_val_temp` etc.
