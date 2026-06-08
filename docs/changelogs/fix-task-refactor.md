# fix/task-refactor

Refactored scout_screen task responsibilities, consolidated shared constants, and added
rolling-average statistics to the STREAM monitor output.

## Removed files

- `scout_screen/components/lvgl_port/ui.c` / `ui.h` — joystick logic had no reason to
  live outside `lvgl_port`; inlining it removed a circular dependency direction

## Architecture changes

- `lvgl_port.c` now owns the joystick CMD state directly (`static volatile uint8_t s_cmd`)
  and exposes `lvgl_port_get_cmd()` so `render_run` needs no LVGL knowledge
- `render.c` replaced `ui_get_cmd()` / `ui_input_event()` / `ui_input_release()` with
  `lvgl_port_get_cmd()` and `lvgl_port_ui_update()`

## Protocol constants

- `CAM_W 640` and `CAM_H 480` moved from `render.c` to `rc_protocol.h`; both nodes now
  derive frame resolution from the same authoritative source
- `CAM_X` / `CAM_Y` remain as local `#define` in `render.c` (screen layout, not protocol)

## Statistics

- `frame_buf.c` gained fixed-size ring buffers (`STAT_WINDOW = 128`, power-of-2) for
  transfer time, decode time, frame size, and inter-frame interval
- Rolling averages are cached as `int32_t` after each push so `monitor_run` reads a
  cheap snapshot without touching the rings
- FPS is derived from the inter-frame interval ring: `fps_tenths = 10000 × count / sum_ms`
- `stream_stats_t` gained `avg_frame_bytes`, `avg_transfer_ms`, `avg_decode_ms`, `fps_tenths`
- `monitor_cmds.c` STREAM output updated to show last and average values plus FPS

## Logging

- Loop-body and per-frame messages downgraded from `ESP_LOGI` to `ESP_LOGD` across all
  `scout_screen/main/` files; `ESP_LOGI` reserved for one-time init confirmations
- Function order in `monitor.c` corrected: `monitor_init` declared before `monitor_run`

## Buffer sizing

- `udp_set_rcvbuf` in `stream_run` now uses `MAX_FRAGS * PKT_MAX` instead of a magic number

## Documentation

- `docs/scout_screen_flow.md` updated: `lvgl_port` row reflects new `get_cmd` API and
  `rc_protocol` dependency; `frame_buf` row describes rolling-average ring buffers;
  `render_run` rc_protocol entry includes `CAM_W`, `CAM_H`
- `docs/scout_cam_flow.md` created: startup sequence, task overview, full dependency
  graph, per-task dependency tables, component dependency table

---

## scout_cam refactor — same structure and rules as scout_screen

### New components

- `components/wifi_sta/` — moved from `main/`; its CMakeLists now owns all SDK deps
  (`esp_wifi`, `nvs_flash`, `esp_event`, `esp_netif`, `esp_timer`); includes the
  timer-based WiFi retry fix (1 s delay via `esp_timer_start_once` on disconnect)
- `components/wdt/watchdog.c/h` — thin wrapper over `esp_task_wdt` so task files
  never include SDK headers directly

### New file layout

- `main/adapters/` created; `frag_tx` and `motor_cmd` moved there
- `motor_task.c/h` moved out of `components/motor_driver/` into `main/` as a task file;
  `motor_driver` component is now a pure GPIO driver (`l298n.c/h`)

### File and function renames

| Old | New |
|---|---|
| `components/motor_driver/motor.c/h` | `l298n.c/h` |
| `components/wdt/wdt.c/h` | `watchdog.c/h` |
| `main/motor_ctrl.c/h` | `motor.c/h` |
| `main/udp_stream.c/h` | `stream.c/h` |
| `motor_ctrl_init / motor_ctrl_run` | `motor_init / motor_run` |
| `udp_stream_init / udp_stream_run` | `stream_init / stream_run` |
| `motor_init / motor_apply` (l298n driver) | `l298n_init / l298n_apply` |
| `wdt_register / wdt_reset` | `watchdog_register / watchdog_reset` |

### Deleted

- `main/monitor.c/h` — stub that was never called from `main.c`
- `main/telemetry.c/h` — stub that was never called from `main.c`
- `components/motor_driver/motor_task.c/h` — replaced by `main/motor.c/h`

### CMakeLists.txt

- `main/CMakeLists.txt` REQUIRES now lists only project components; SDK packages
  (`esp_wifi`, `nvs_flash`, etc.) removed — they belong in their wrapping component
- `motor_driver/CMakeLists.txt` reduced to `l298n.c` only

### Coding style violations fixed

- Function order: `_init` before `_run` in all task files
- `udp_stream_init` was a no-op; `stream_init` now sets the destination address and logs
- `motor_cmd` TAG changed from `"motor"` to `"motor_cmd"` (three files shared the same TAG)

### Final consistency pass

- `frag_tx` API cleaned up: destination address moved from parameter to internal
  `static struct sockaddr_in s_dest`; new `frag_tx_init(ip, port)` stores it.
  `#include "lwip/sockets.h"` removed from `frag_tx.h` — struct type no longer leaks
  into callers. `stream.c` calls `frag_tx_init` in `stream_init()` instead.
- `camera_init()` changed from `esp_err_t` to `void`; `ESP_ERROR_CHECK` now internal;
  `ESP_LOGI(TAG, "camera ready")` added; comment block added to `camera.c` explaining
  the module's role (hides `camera_fb_t` from task code).
- `camera.h` no longer includes `esp_err.h`.
- `main.c` startup log removed; `esp_err.h` include removed; `camera_init()` call
  simplified; `vTaskDelete(NULL)` added — matches `scout_screen/main/main.c` pattern.
- `docs/scout_cam_flow.md` stale folder references corrected (`motor_driver/` →
  `l298n/`, `wdt/` → `watchdog/`).
