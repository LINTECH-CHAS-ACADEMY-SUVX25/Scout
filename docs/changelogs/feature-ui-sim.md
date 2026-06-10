# feature/ui-sim — delad UI-kod

UI:t låg som en kopia i `sim/ui.c` och skulle klistras tillbaka i
`lvgl_port.c` för hand när man var nöjd. Det höll redan på att glida isär
(olika telemetrirader, TCP/UDP-etikett, olika `make_obj`). Nu kompilerar
firmware och simulatorn samma fil i stället, så kopieringssteget försvinner
helt — ser det rätt ut i simulatorn är enhetskoden redan uppdaterad.

## Tillagt

- `lvgl_port_ui.c`/`.h` i `scout_screen/components/lvgl_port/` — hela
  UI-layouten (sim-designen: mörkt tema, cyan accent, Press Start 2P).
  Filen får inte inkludera ESP-IDF- eller FreeRTOS-headers, annars bryts
  PC-bygget
- Fontfilerna `press_start_2p_8.c`/`_24.c` flyttade från `sim/` in i
  komponenten och tillagda i `SRCS`
- `LV_LVGL_H_INCLUDE_SIMPLE` i komponentens compile definitions —
  de genererade fontfilerna inkluderar `lvgl.h` via den guarden

## Ändrat

- `lvgl_port.c` innehåller nu bara driver-delen (flush, touch, tick, init,
  render). `lvgl_port.h` inkluderar `lvgl_port_ui.h`, så `render.c` och
  `main.c` är orörda
- `s_cmd` och `lvgl_port_get_cmd()` bor i UI-filen — joysticken äger statet
- Enhetens UI byter till sim-designen: mörkt tema, pixelfont, UDP-etikett
  i bottenfältet
- sim: `sim_screen.h` heter nu `display.h` så den delade filens
  `#include "display.h"` löser på båda sidor. `-I.` står före
  komponent-katalogen i `Makefile` så simens `lv_conf.h`/`display.h`
  skuggar enhetens
- `sim/Makefile` bygger de delade filerna under `build/port/`
- `sim/README.md` omskriven — beskriver delad källfil i stället för
  kopiera-tillbaka-flödet

## Borttaget

- `sim/ui.c` (kopian med KOPIERA-markörerna)
- `cmd`- och `uptime`-raderna i telemetrin plus hex-uppdateringen i
  `joy_event` — fanns kvar på enheten men togs bort i sim-designen,
  så de följer med ut här
- `LV_FONT_MONTSERRAT_48` i enhetens `lv_conf.h` — bara gamla intron
  använde den
- Oanvänd `make_bar()` som låg kvar i `sim/ui.c`

## Verifierat

- `make` i `sim/` bygger rent, skärmdump kontrollerad (intro spelar och
  tar bort sig själv, huvudskärmen hel)
- `idf.py build` i `scout_screen/` ok, 44 % fritt i app-partitionen

## Kvarstår

- `SCREEN_W`/`SCREEN_H` i `sim/display.h` måste hållas i synk med
  `scout_screen/components/display/display.h` för hand
