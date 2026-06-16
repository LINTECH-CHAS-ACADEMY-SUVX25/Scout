# refactor/ui-component-split

## Tillagt
- `topbar.c/h`, `botbar.c/h`, `tele.c/h`, `joy.c/h`, `cam.c/h`, `config.c/h`, `menu.c/h`, `intro.c/h`, `themes.c/h` — en fil per UI-komponent istället för en monolitisk `scout_ui.c`
- `layout.h` — alla layout-konstanter (`BAR_H`, `PANEL_GAP`, `SIDE_W`, etc.) separerade från deklarationer
- `internal.h` — modul-intern header med delad `s_root`-extern och hjälpfunktioner (`make_obj`, `make_panel`, etc.)
- Tre nya delade LVGL-stilar i `themes.c/h`: `st_fill_bad`, `st_fill_good`, `st_chip_on`
- `@brief` Doxygen-kommentarer på alla interna modul-funktioner i komponenternas `.h`-filer

## Ändrat
- `scout_ui.c` — nu en ren koordinator; bygger UI genom att anropa varje komponents `build()`-funktion
- `joy.c` — `update_cmd_badges()` använder `LV_STATE_USER_1` + delade stilar istället för direkta `lv_obj_set_style_*`-anrop; temabyte hanteras automatiskt av LVGL
- `topbar.c` — `s_wifi_dot`, `s_link_dot` och `s_link_lbl` använder `LV_STATE_USER_1` + stilar; `scout_ui_update()` togglar state istället för att sätta färger direkt
- `CMakeLists.txt` — uppdaterad med nya filnamn
- `sim/Makefile` — wildcard ändrat från `scout_ui*.c` till `*.c`

## Borttaget
- `joy_refresh_theme()` — onödig efter övergång till state-baserad omfärgning; `lv_obj_report_style_change(NULL)` i `scout_ui_set_theme()` räcker
