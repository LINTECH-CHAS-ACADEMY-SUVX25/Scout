# Scout UI-simulator

Kör skärmens LVGL-UI på en PC via SDL2, så att UI-arbetet kan göras utan
att flasha hårdvara. Bygger LVGL **och UI-koden** från samma källträd som
`scout_screen` använder (`scout_screen/components/lvgl_port/`) — det finns
ingen kopia att hålla i synk; en ändring i `lvgl_port_ui.c` syns i både
simulatorn och firmware-bygget.

## Bygg och kör

```sh
cd sim
make run
```

Första bygget tar en stund (hela LVGL kompileras). Därefter byggs bara det
som ändrats. `make clean` tömmer `build/`.

Tangenter i fönstret:
* **c** — stega wifi-signalnivån 0-3 (testar `lvgl_port_ui_update`)
* **t** — stega färgtemat CYAN → AMBER → GREEN (samma byte som
  THEMES-dropdownen i topbaren gör)
* **q** / **Esc** — avsluta

Joysticken styrs med musen, precis som touch på enheten.

Vid start visas intro-overlayn (`lvgl_port_intro_screen`) med boot-stegen
WIFI → MONITOR → STREAM → READY. På enheten kommer stegen från `app_main`
mellan de riktiga init-anropen; simulatorn fejkar dem med 1 s mellanrum
(`INTRO_STEP_MS` i `main.c`). 1,2 s efter sista steget tar overlayn bort
sig själv och visar UI:t — totalt ~5,2 s i simulatorn.

Ändrar du `lv_conf.h` (t.ex. aktiverar en font): kör `make clean` först —
LVGL-objekten spårar inte `lv_conf.h` och byggs annars inte om.

Headless skärmdump: `SDL_VIDEODRIVER=offscreen SIM_SHOT=ut.bmp ./sim`. Lägg
till `SIM_SHOT_MS=5500` för att spola fram förbi intron först, och
`SIM_THEME=0|1|2` för att byta tema innan dumpen.

## Så här hänger det ihop

| Fil            | Roll                                                        |
|----------------|-------------------------------------------------------------|
| `main.c`       | SDL + LVGL-glue + kamerareferensram. Enbart för simulatorn  |
| `lv_conf.h`    | PC-variant av enhetens `lv_conf.h`                          |
| `display.h`    | `SCREEN_W` / `SCREEN_H` (ersätter enhetens `display.h`)     |

Delade filer som kompileras från `scout_screen/components/lvgl_port/`:

| Fil            | Roll                                                        |
|----------------|-------------------------------------------------------------|
| `lvgl_port_ui.c` | Hela UI-layouten — samma fil som enheten kompilerar       |
| `press_start_2p_8.c`  | UI-fonten (genererad C-font, Press Start 2P 8px)    |
| `press_start_2p_96.c` | Intro-loggans font (96px, endast mellanslag + A–Z)  |

## Font

UI:t använder **Press Start 2P** (`press_start_2p_8.c`), en retro 8-bitars
pixel-font, genererad från TTF med
[`lv_font_conv`](https://github.com/lvgl/lv_font_conv). Den renderas i
**bpp 1** (ingen antialiasing) så pixlarna förblir skarpa, och i en storlek
som är multipel av fontens 8px-rutnät. Regenerera så här — viktigt:
`--no-compress`, annars krävs `LV_USE_FONT_COMPRESSED` i `lv_conf.h`:

```sh
npm install -g lv_font_conv
lv_font_conv --font PressStart2P-Regular.ttf \
  --size 8 --bpp 1 --format lvgl --no-compress \
  --range 0x20-0x7E --lv-font-name press_start_2p_8 -o press_start_2p_8.c
```

Press Start 2P är OFL-licensierad — behåll licensfilen om fonten checkas in.

Kamerarutan ritas som en cyan referensram på **480 × 480**, centrerad. På
enheten finns ingen kamera-widget — videon blittas dit förbi LVGL av
`render.c`. Ramen är bara där så layouten kan ritas runt rätt yta.

## Delad UI-kod

UI-layouten ligger i `scout_screen/components/lvgl_port/lvgl_port_ui.c` och
kompileras av båda byggena — det finns inget kopieringssteg. När simulatorn
ser rätt ut är firmware-koden redan uppdaterad; verifiera med `idf.py build`
i `scout_screen/`.

`lvgl_port_ui.c` får inte inkludera ESP-IDF- eller FreeRTOS-headers (då
bryts sim-bygget). Driver-delen (`flush_cb`, touch, tick, `lvgl_port_init`,
`lvgl_port_render_frame`) lever i `lvgl_port.c` på enheten respektive i
`main.c` i simulatorn.

Simulatorns `display.h` skuggar enhetens (via include-ordningen i
`Makefile`) och måste hålla `SCREEN_W`/`SCREEN_H` i synk med
`scout_screen/components/display/display.h`.
