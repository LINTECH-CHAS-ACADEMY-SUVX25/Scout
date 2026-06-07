# fix/restructure-code

Rewrote every project-owned file to conform to `docs/CODING_STYLE.md`. No logic was changed.

## File renames

- `scout_screen/main/wifi.c/h` → `wifi_ap.c/h`
- `scout_cam/main/wifi.c/h` → `wifi_sta.c/h`

## New files

- `scout_screen/main/uart_console.c/h` — UART0 adapter; wraps `driver/uart.h` so task files do not call drivers directly

## Formatting fixes (all files)

- `if(`, `while(`, `for(` — no space before `(`
- Struct initialisers use inline `{`; only function definitions put `{` on its own line
- `/* */` comments replaced with `//` throughout
- Include order: own header, project modules, FreeRTOS, ESP-IDF, stdlib

## Architecture fixes

- `scout_screen/main/monitor.c` — removed `driver/uart.h` and `esp_wifi.h` (task calling driver directly); replaced with `uart_console_*` and `wifi_ap_sta_count()`
- `scout_screen/components/lvgl_port/lvgl_port.c` — replaced Waveshare `EXAMPLE_LCD_H/V_RES` with `SCREEN_W`/`SCREEN_H` from `display.h`

## API additions

- `wifi_ap_sta_count()` added to `wifi_ap.h/c`
- `uart_console_write()` (no newline) added alongside `uart_console_println` / `uart_console_printfln`

## Constants extracted

- `SCREEN_W 1024`, `SCREEN_H 600` added to `display/display.h` (previously magic numbers)
- `JOY_RADIUS 52` added to `lvgl_port.c` (previously magic `52.0f`)

## CMakeLists.txt updates

- `scout_cam/main/CMakeLists.txt` — `wifi.c` → `wifi_sta.c`
- `scout_screen/main/CMakeLists.txt` — `wifi.c` → `wifi_ap.c`; added `uart_console.c`
