# Coding style

## Architecture
Scout uses a strictly layered architecture. Each layer may only call the layer directly below it.
```
tasks       stream.c  render.c  monitor.c
                ↓         ↓         ↓
adapters    udp.c  display.c  uart_console.c  wifi.c  jpeg_decode.c
                ↓         ↓         ↓               ↓
drivers     lwip   rgb_lcd   driver/uart        esp_wifi
```
* Task files must never include SDK headers (`driver/`, `esp_wifi.h`, `lwip/`) directly
* `CMakeLists.txt` REQUIRES lists only project components and shared components — SDK packages (`esp_wifi`, `nvs_flash`, `esp_netif`, `lwip`) belong in the component that wraps them, not in `main`'s REQUIRES
* Waveshare files (`i2c.c`, `gt911.c`, `rgb_lcd_port.c`, `io_extension.c`) are external — do not modify them
* Each task owns its data; other tasks access it through public functions, never shared globals
* Shared state accessed by more than one task must be protected with a mutex
* A module owns its own state — do not create a satellite file whose only purpose is to hold state on behalf of another module; merge the state into the module that is responsible for it

## File types
Every file has a type that describes its role in the architecture:
* *entry point* — `app_main`, initialises everything and starts tasks
* *task* — owns a FreeRTOS task and its data
* *driver* — wraps SDK or hardware directly (esp_wifi, i2c_master, RGB panel)
* *adapter* — thin translation layer between two modules
* *wrapper* — thin wrapper over a library or SDK call with no added logic
* *facade* — combines multiple modules into one simplified API
* *port* — connects one framework to another (e.g. LVGL to display + touch)
* *library* — generic utility with no project-specific knowledge
* *protocol* — header-only file defining shared constants or a communication contract
* *interface* — abstract class defining a contract for an implementation to fulfill
* *mock* — implements an interface for use in tests only

## File placement
* `shared_components/` — the file is included by both scout_cam and scout_screen
* `components/` — the file does not import any module from `main/`
* `main/` — the file imports one or more modules from `main/`, or is application-specific
* `main/adapters/` — adapter files that translate between a task and a component or shared module; task files (`stream.c`, `render.c`, `monitor.c`) stay at the top level of `main/`

A file that imports nothing from `main/` and wraps only SDK or shared components belongs in `components/`, not `main/`, even if it is currently only used by one project.

## File structure
A `.c` file follows this order:
1. Own header (`"module.h"`)
2. Other project module headers (`"stream.h"`, `"udp.h"`)
3. FreeRTOS headers (`"freertos/FreeRTOS.h"`)
4. ESP-IDF headers (`"esp_log.h"`, `"driver/uart.h"`)
5. Standard library headers (`<string.h>`, `<stdint.h>`)
6. Module-level `//` comment — required for *task*, *facade*, and *port* files; optional for *driver* and *adapter* if the behaviour is non-obvious; omit for all others
7. `#define` constants
8. `static const char *TAG = "module";`
9. Static variables (`s_` prefix)
10. Private function implementations (defined before their first call site)
11. Public function implementations

A `.h` file follows this order:
1. `#pragma once`
2. Includes required for the declared types
3. Type definitions (`_t` structs)
4. Public function declarations with `//` comments

## Formatting
Function definition brackets open on a new line. Everything else (`if`, `while`, `for`, struct initialisers, enums) opens on the same line. No space between a keyword and `(`:
```c
static bool try_send(const char *msg)
{
    if(msg == NULL) return false;

    for(int i = 0; i < RETRY_MAX; i++) {
        if(uart_write(msg) == ESP_OK) return true;
    }
    return false;
}
```

## Naming conventions
* Words are separated by `_`
* Functions and variables use only lower case letters (e.g. *frame_count*)
* Defines and enum values use only upper case letters (e.g. *FRAME_MAX*, *CMD_FORWARD*)
* Public function names are prefixed with their module name: *stream_init*, *wifi_ap_sta_count*
* File-scope static variables are prefixed with `s_`: `static uint32_t s_frame_count`
* Typedefs always end with `_t`: *stream_stats_t*
* File names follow the pattern `subject` or `subject_qualifier`:
  * The qualifier describes the role or operation — never the file type (`_driver`, `_task`, `_wrapper` are forbidden as suffixes)
  * Add a qualifier only when needed to distinguish the file from others with the same subject
  * Examples: *wifi_ap* (role), *jpeg_decode* (operation), *uart_console* (role), *gt911* (chip name, no qualifier needed)

## Coding guide
* Functions:
  * Functions must be shorter than 50 lines
  * Never exceed 200 lines except for trivially sequential code
  * A function with section-label comments must be split into smaller functions
* Variables:
  * One line, one declaration (BAD: `char x, y;`)
  * Use `<stdint.h>` types (*uint8_t*, *int32_t* etc.)
  * Declare variables where needed, not all at the top of a function
  * Use the smallest required scope
  * Variables in a file (outside functions) are always *static*
  * Do not use global variables — use functions to get/set static data

## Comments
Use `//` for all comments, never `/* */`. Write *why*, not *what*:
```c
// Backspace — erase the last character on the terminal.
if(ch == 0x7F || ch == 0x08) { ... }
```

## Header files
The `.h` file is the public contract for a module:
* Document every public function with a `//` comment
* Include only what is needed for the types in the declarations
* Internal (`static`) functions belong in `.c` only

## Constants
Constants are placed based on what they describe, not where they happen to be used:
* Must match between cam and screen (CMD_*, ports, FRAME_MAGIC) → `shared_components/rc_protocol/rc_protocol.h`
* Describes a module's hardware or public contract → that module's `.h`, even if only one file currently uses it
* Implementation detail of one file → that file's `.c`

Never duplicate a constant that already exists in `rc_protocol.h`. Never use magic numbers in logic:
```c
if(mag > JOY_RADIUS) { ... }  // Good
if(mag > 52.0f)       { ... }  // Bad
```

## Error handling
Three patterns — which one to use is determined by where in the stack the code lives.

**Initialisation** — `ESP_ERROR_CHECK` is acceptable; a crash at boot is recoverable:
```c
ESP_ERROR_CHECK(esp_wifi_start());
```
**Adapters** — return `bool` or `esp_err_t` and log:
```c
if(uart_driver_install(...) != ESP_OK) {
    ESP_LOGE(TAG, "uart_driver_install failed");
    return false;
}
```
**Tasks** — log and continue; never crash inside a running task:
```c
if(!fb) {
    ESP_LOGE(TAG, "camera capture failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    continue;
}
```

## Memory
Do not call `malloc` or `free` inside a running task loop. Allocate once during initialisation:
```c
s_asm_buf = heap_caps_malloc(FRAME_MAX, MALLOC_CAP_SPIRAM);  // Good — once in stream_init()
uint8_t *buf = malloc(len);                                   // Bad — every iteration
```
Use `MALLOC_CAP_SPIRAM` for large buffers. Use internal SRAM for anything timing-critical.

## Logging
* `ESP_LOGE` — an error that stops or breaks something
* `ESP_LOGW` — unexpected but execution continues
* `ESP_LOGI` — one confirmation line per module at init time
* `ESP_LOGD` — debug detail, compiled out in release via `CONFIG_LOG_DEFAULT_LEVEL`
