# Scout UI simulator

Runs the screen's LVGL UI on a PC via SDL2 so UI work can be done without
flashing hardware. Builds LVGL **and the UI code** from the same source tree
that `scout_screen` uses — there is no copy to keep in sync; a change to
`scout_ui.c` is immediately reflected in both the simulator and the firmware
build.

## Build and run

```sh
cd sim
make run
```

The first build takes a while (all of LVGL is compiled). After that only
changed files are rebuilt. `make clean` wipes `build/`.

Keys in the window:

* **c** — cycle WiFi signal level 0–3 (exercises `scout_ui_update`)
* **t** — cycle colour theme SONAR → DESERT → NIGHT OPS (same byte the THEMES
  dropdown in the top bar uses)
* **q** / **Esc** — quit

The joystick is controlled with the mouse, just like touch on the device.

At startup the intro overlay (`scout_ui_intro_screen`) runs through the boot
steps WIFI → MONITOR → STREAM → READY. On the device these steps come from
`app_main` between the real init calls; the simulator fakes them 1 s apart
(`INTRO_STEP_MS` in `main.c`). 1.2 s after the last step the overlay removes
itself and reveals the UI — about 5.2 s total in the simulator.

If you change `lv_conf.h` (e.g. enable a font): run `make clean` first —
LVGL objects do not track `lv_conf.h` and will not be rebuilt otherwise.

## File overview

Simulator-only files:

| File         | Role                                                      |
|--------------|-----------------------------------------------------------|
| `main.c`     | SDL + LVGL glue + camera reference box                    |
| `lv_conf.h`  | PC variant of the device's `lv_conf.h`                    |
| `display.h`  | `SCREEN_W` / `SCREEN_H` (shadows the device's `display.h`)|

Shared files compiled from `scout_screen/components/ui/`:

| File                    | Role                                              |
|-------------------------|---------------------------------------------------|
| `scout_ui.c`            | Full UI layout — the same file the device builds  |
| `fonts/press_start_2p_8.c`  | UI font (generated C font, Press Start 2P 8 px) |
| `fonts/press_start_2p_24.c` | Mid-size variant (24 px)                        |
| `fonts/press_start_2p_96.c` | Intro label font (96 px, space + A–Z only)      |

## Font

The UI uses **Press Start 2P** (`press_start_2p_8.c`), a retro 8-bit pixel
font generated from a TTF with
[`lv_font_conv`](https://github.com/lvgl/lv_font_conv). It is rendered at
**bpp 1** (no antialiasing) so pixels stay sharp, and at sizes that are
multiples of the font's 8 px grid. To regenerate — note `--no-compress`,
otherwise `LV_USE_FONT_COMPRESSED` must be enabled in `lv_conf.h`:

```sh
npm install -g lv_font_conv
lv_font_conv --font PressStart2P-Regular.ttf \
  --size 8 --bpp 1 --format lvgl --no-compress \
  --range 0x20-0x7E --lv-font-name press_start_2p_8 -o press_start_2p_8.c
```

Press Start 2P is OFL-licensed — keep the licence file if the font is checked
in.

## Camera box

The camera area is drawn as a cyan reference frame at **480 × 480**, centred.
On the device there is no camera widget — video is blitted there directly by
`render.c`, bypassing LVGL. The box exists only so the layout can be designed
around the correct area.

## Shared UI code

The UI layout lives in `scout_screen/components/ui/scout_ui.c` and is compiled
by both builds — there is no copy step. When the simulator looks right, the
firmware code is already updated; verify with `idf.py build` in
`scout_screen/`.

`scout_ui.c` must not include ESP-IDF or FreeRTOS headers (that would break
the sim build). The driver layer (`flush_cb`, touch, tick, `lv_disp_drv`
setup) lives in the device's `lvgl_port.c` and in `main.c` in the simulator.

The simulator's `display.h` shadows the device's via the include order in the
`Makefile` and must stay in sync with
`scout_screen/components/display/display.h`.
