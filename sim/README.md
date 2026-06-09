# Scout UI-simulator

Kör skärmens LVGL-UI på en PC via SDL2, så att UI-arbetet kan göras utan
att flasha hårdvara. Bygger LVGL från samma vendored källträd som
`scout_screen` använder.

## Bygg och kör

```sh
cd sim
make run
```

Första bygget tar en stund (hela LVGL kompileras). Därefter byggs bara det
som ändrats. `make clean` tömmer `build/`.

Tangenter i fönstret:
* **c** — växla connected/waiting (testar `lvgl_port_ui_update`)
* **q** / **Esc** — avsluta

Joysticken styrs med musen, precis som touch på enheten.

Vid start spelas intro-overlayn (`lvgl_port_intro_screen`) i ~2,5 s och tar
sedan bort sig själv och visar UI:t.

Ändrar du `lv_conf.h` (t.ex. aktiverar en font): kör `make clean` först —
LVGL-objekten spårar inte `lv_conf.h` och byggs annars inte om.

Headless skärmdump: `SDL_VIDEODRIVER=offscreen SIM_SHOT=ut.bmp ./sim`. Lägg
till `SIM_SHOT_MS=3000` för att spola fram förbi intron först.

## Så här hänger det ihop

| Fil            | Roll                                                        |
|----------------|-------------------------------------------------------------|
| `ui.c`         | UI-vyn — **spegling** av UI-sektionen i `lvgl_port.c`       |
| `main.c`       | SDL + LVGL-glue + kamerareferensram. Enbart för simulatorn  |
| `lv_conf.h`    | PC-variant av enhetens `lv_conf.h`                          |
| `sim_screen.h` | `SCREEN_W` / `SCREEN_H` (motsvarar `display.h` på enheten)  |
| `press_start_2p_8.c`  | UI-fonten (genererad C-font, Press Start 2P 8px)    |
| `press_start_2p_24.c` | Intro-loggans font (Press Start 2P 24px)            |

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

## Flytta UI:t till hårdvaran

`ui.c` är skriven för att kopieras rakt tillbaka. Allt mellan markörerna

```
// ===== KOPIERA FRÅN HÄR =====
...
// ===== KOPIERA TILL HÄR =====
```

är ordagrant identiskt med UI-sektionen i
`scout_screen/components/lvgl_port/lvgl_port.c`. När du är nöjd: ersätt
motsvarande block i `lvgl_port.c` med ditt. Raderna utanför markörerna
(includes överst, `sim_ui_init` längst ned) är simulator-specifika och ska
**inte** med.

Driver-delen (`flush_cb`, touch, tick, `lvgl_port_init`,
`lvgl_port_render_frame`) rörs aldrig — den lever bara i `lvgl_port.c` på
enheten respektive i `main.c` i simulatorn.

När du flyttar över, kom ihåg att enheten också behöver:
* `press_start_2p_8.c` och `press_start_2p_24.c` (intro-loggan) i
  `components/lvgl_port/` (lägg till i komponentens `CMakeLists.txt` `SRCS`)
* `LV_LVGL_H_INCLUDE_SIMPLE` definierad (annars `#include "lvgl/lvgl.h"` i
  fontfilen), eller justera fontfilens include
* `CAM_W`/`CAM_H` → 480 i `rc_protocol.h` och bottombar-höjd 32→36 (BAR_H)
  för att matcha sim-layouten
