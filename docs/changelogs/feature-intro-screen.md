# feature/intro-screen

Animerad intro-skärm som visas vid uppstart och tar bort sig själv när
animationen är klar. Mergad mot den omstrukturerade main (adapter layers).

## Tillagt

- `lvgl_port_intro_screen()` — svart overlay med "SCOUT"-logga, animerad
  laddningsstapel (2,5 s ease-out) och "LOADING..."-text
- `ui_intro_screen()` som anropar porten, samt anrop i `app_main`
- `LV_FONT_MONTSERRAT_48` och `LV_DRAW_COMPLEX` i `lv_conf.h`
- Custom partition table (`partitions.csv`) i `sdkconfig.defaults`

## Ändrat vid merge

- Intro-koden anpassad till nya arkitekturen: `EXAMPLE_LCD_H/V_RES` →
  `SCREEN_W`/`SCREEN_H`, `INTRO_BAR_W/H`-defines flyttade till filtoppen
- `scout_cam/dependencies.lock` behåller mains version (nyare deps)

## Borttaget

- Oanvänd `make_bar()` i `lvgl_port.c`
- `LV_FONT_UNSCII_16` (aktiverad men aldrig använd)

## Kvarstår

- `ui_intro_screen()` anropas efter `render_init()`, vilket skapar
  LVGL-objekt från `app_main` medan render-tasken kör LVGL på core 1.
  Bör flyttas före render-tasken startar. Fixas separat.
