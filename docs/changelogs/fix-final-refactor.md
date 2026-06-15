# Changelog: fix/final-refactor

Issue #60 (Namngivning och single-responsibility-genomgång) execution sequence.

## Step 1 — move `ring_buffer` to `components/`

- Moved `scout_screen/main/adapters/ring_buffer.{c,h}` →
  `scout_screen/components/ring_buffer/ring_buffer.{c,h}` (contents unchanged). It is a
  generic int32 stats ring with no project knowledge, so `components/` is the correct home
  per `docs/CODING_STYLE.md`.
- Added `scout_screen/components/ring_buffer/CMakeLists.txt` (`SRCS ring_buffer.c`,
  `INCLUDE_DIRS "."`, no REQUIRES — only uses `<stdint.h>`).
- `scout_screen/main/CMakeLists.txt`: removed `adapters/ring_buffer.c` from SRCS, added
  `ring_buffer` to REQUIRES. Only consumer `screen_state.{c,h}` is unchanged.
- Verified: scout_screen builds.

## Step 2 — relocate `jpeg` to `scout_screen/components/`; remove dead `esp_new_jpeg` from cam

- Moved `shared_components/jpeg/{jpeg.c,jpeg.h,CMakeLists.txt}` →
  `scout_screen/components/jpeg/` via `git mv` (contents unchanged). `jpeg` is screen-only —
  cam captures raw JPEG and never includes `jpeg.h`.
- Added `scout_screen/components/jpeg/idf_component.yml` declaring `espressif/esp_new_jpeg`
  so the managed dependency travels with its only consumer.
- `scout_screen/main/idf_component.yml`: removed `espressif/esp_new_jpeg` (now owned by the
  `jpeg` component).
- `scout_cam/main/idf_component.yml`: removed dead `espressif/esp_new_jpeg` dependency
  (`espressif/esp32-camera` uses a separate `esp_jpeg`/TJpgDec component, unaffected).
- `scout_cam/CMakeLists.txt`: added `set(COMPONENTS main)` so cam builds only `main` and its
  transitive dependencies, excluding `jpeg`, `scout_hal`, `unity`, and `cmock`.
- Verified: both scout_cam and scout_screen build.

## Step 3 — rename adapters: motor_queue, rc_tx, frame_pool

Pure renames — no logic changes.

- `scout_cam/main/adapters/motor_cmd.{c,h}` → `motor_queue.{c,h}` (git mv).
  All symbols renamed: `motor_cmd_init/send/recv` → `motor_queue_init/send/recv`.
  Updated callers: `cam_state.c` (3× send), `motor.c` (init + recv), `stream.c` (1× send).
  `scout_cam/main/CMakeLists.txt`: `adapters/motor_cmd.c` → `adapters/motor_queue.c`.
- `scout_screen/main/adapters/cam_cmd.{c,h}` → `rc_tx.{c,h}` (git mv).
  All symbols renamed: `cam_cmd_*` → `rc_tx_*` (init, bind, learn, send, send_throttled).
  Updated callers: `stream.c` (init/bind/learn), `render.c` (send_throttled).
  `scout_screen/main/CMakeLists.txt`: `adapters/cam_cmd.c` → `adapters/rc_tx.c`.
- `scout_screen/main/adapters/frame_buf.{c,h}` → `frame_pool.{c,h}` (git mv).
  All symbols renamed: `frame_buf_*` → `frame_pool_*` (init, asm, pkt, publish,
  try_acquire, release).
  Updated callers: `stream.c` (init/pkt/publish), `render.c` (try_acquire/release),
  `frag_rx.c` (asm). Stale `frame_buf` references in comments also updated.
  `scout_screen/main/CMakeLists.txt`: `adapters/frame_buf.c` → `adapters/frame_pool.c`.
- Verified: both scout_cam and scout_screen build.

## Step 5 — monitor tree restructure; promote `uart_console` to adapter; delete `cam_diag_fmt`

### Fold cam_diag task into stream_run

- Removed `adapters/cam_diag.{c,h}` and its task (`cam_diag_init` / `cam_diag_run`).
- `stream_run` now opens a second socket (`diag_sock = udp_open(DIAG_PORT)`) alongside the
  video socket. At the bottom of each loop iteration it calls `udp_try_recv` non-blocking; on
  a full `cam_diag_pkt_t` it forwards to `screen_state_set_cam`. Polling every ~1 s is enough
  for telemetry that arrives every 2 s.
- `monitor_init` no longer calls `cam_diag_init`.
- `CMakeLists.txt`: removed `adapters/cam_diag.c` from SRCS.

### Promote `uart_console` component → `adapters/console`

- Deleted `scout_screen/components/uart_console/` (uart_console.c, uart_console.h,
  CMakeLists.txt).
- Created `scout_screen/main/adapters/console.{c,h}`. All public symbols use the `term_`
  prefix (`term_init`, `term_write`, `term_println`, `term_printfln`, `term_read_byte`,
  `term_try_getchar`, `term_read_line`, `term_run_handler`, `term_dispatch`,
  `term_handler_t`). The `console_` prefix was taken by ESP-IDF's `esp_stdio` component
  which defines `console_write` with C linkage internally.
- All monitor command handlers and formatters that were in `monitor_cmds.c` moved into
  `console.c`. `STREAM_LINE_COUNT` is now a local `#define` in `console.c`.
- `monitor.c` reduced to `monitor_init()` + static `monitor_run()` only — no logic.
- `CMakeLists.txt`: removed `uart_console` from REQUIRES, added `esp_driver_uart`; added
  `adapters/console.c` to SRCS.

### Delete `cam_diag_fmt` component; inline formatters

- Deleted `scout_screen/components/cam_diag_fmt/` (cam_diag_fmt.c, cam_diag_fmt.h,
  CMakeLists.txt). The component held three trivial `snprintf` wrappers (`fmt_temp`,
  `fmt_humi`, `fmt_pres`).
- Both consumers now carry their own `static` copies:
  - `adapters/console.c` (used by `cmd_camdiag`)
  - `components/ui/scout_ui.c` (used as callbacks in the `tele_field_t` table)
- `scout_ui.c`: removed `#include "cam_diag_fmt.h"`, added `#include <stdio.h>`.
- `components/ui/CMakeLists.txt`: removed `cam_diag_fmt` from REQUIRES.

- Verified: scout_screen builds.

## Step 6 — split `screen_state` → `screen_state` + `screen_stats`

- New `scout_screen/main/adapters/screen_stats.{c,h}`. Owns everything timing/metrics:
  `tick_slot_t`, `screen_tick_t`, 9 `ring_buf_t` statics, `screen_stats_t` (renamed from
  `screen_state_t`), and `screen_stats_{render,stream}_tick_init / tick / tick_split / get`.
  When the `updates_streaming` slot commits, calls `screen_state_mark_rx_time(now_ms)`
  instead of writing `s_last_rx_ms` directly — one-way dependency, no cycle.
- `screen_state.{c,h}` now owns only: scene FSM (`scene_t`, set/get/name), cam-diag cache
  (`set_cam`, `get_cam`, `cam_dirty_take`), `screen_status_t`, `is_streaming`, `has_streamed`,
  and the new `screen_state_mark_rx_time` setter. Ring-buffer include removed.
- `render.c`: added `screen_stats.h`; renamed `screen_state_render_tick_init → screen_stats_render_tick_init`,
  `screen_state_tick → screen_stats_tick`, `screen_state_tick_split → screen_stats_tick_split` (×3).
- `stream.c`: added `screen_stats.h`; renamed `screen_state_stream_tick_init → screen_stats_stream_tick_init`,
  `screen_state_tick → screen_stats_tick`.
- `console.c`: added `screen_stats.h`; renamed `screen_state_t → screen_stats_t` (4 occurrences),
  `screen_state_get → screen_stats_get` (2 occurrences).
- `CMakeLists.txt`: added `adapters/screen_stats.c` to SRCS.
- Verified: scout_screen builds.

## Task 10 — cam_state SRP audit + camera control protocol

### Dead code cleanup

- Removed `camera_fault` field from `cam_status_t` — it was set in `cam_state_camera_start`
  then immediately clobbered by `esp_restart()`, so it was never observed. The "running degraded"
  log and the `if(cam_status.camera_fault)` branch in `stream_run` were also dead and removed.
- Reboot on repeated camera init failure is intentional and unchanged.

### RC receive extracted to `rc_rx` adapter

- New `scout_cam/main/adapters/rc_rx.{c,h}`. Single responsibility: drain `joy_pkt_t` from
  CMD_PORT, forward each to `motor_queue_send`, update `cam_status.screen_online` /
  `cam_status.streaming`, and count silent frames to detect screen disconnect.
  Mirrors the `rc_tx` adapter on the screen side.
- `cam_state_process_cmds` deleted. The RC-receive block in `cam_state_try_resume` removed;
  that function now only checks WiFi reconnect and takes no socket argument.
- `stream.c` calls `rc_rx_process(sock)` everywhere `cam_state_process_cmds(sock)` was called.

### Camera control protocol

- `rc_protocol.h`: added `CTRL_PORT 3337`, `cam_ctrl_cmd_t` enum (CAMERA_ON/OFF, SENSOR_ON/OFF,
  SET_QUALITY/BRIGHTNESS/CONTRAST/SATURATION/HMIRROR/VFLIP/SPECIAL_EFFECT), and
  `cam_ctrl_pkt_t` (2 bytes: cmd + value).
- `cam_status_t`: added `camera_enabled` (false skips capture and video TX) and
  `sensor_enabled` (false skips BME280 reads and sensor fields in the diag packet).
- `cam_state_init()` sets both flags to true; called from `main.c` before
  `cam_state_camera_start()`.
- `cam_state_apply_ctrl(pkt)` dispatches ON/OFF commands to the status flags and SET_* commands
  to the new `camera_apply_setting()` in the camera adapter.
- `camera.c`: new `camera_apply_setting(cmd, value)` applies a single OV2640 sensor setting via
  `esp_camera_sensor_get()`.
- `stream.c`: opens `ctrl_sock = udp_open(CTRL_PORT)` inline; polls it with `udp_try_recv`
  each loop iteration and calls `cam_state_apply_ctrl` on a full packet — same pattern as
  `diag_sock` on the screen side.
- `CMakeLists.txt`: added `adapters/rc_rx.c` to SRCS.
- Verified: scout_cam builds and flashed successfully.

## Step 7 — fold `telemetry.{c,h}` into `stream_run` (scout_cam)

- Deleted `scout_cam/main/telemetry.c` and `scout_cam/main/telemetry.h`.
- `stream_run` now sends `cam_diag_pkt_t` inline every 2 s: elapsed-time check via
  `esp_timer_get_time()` at the top of the loop (before streaming/fault branches), reusing
  the existing `sock`. BME280 is read at that point; all other fields populated identically
  to the old `telemetry_run`. No second socket needed.
- `main.c`: removed `#include "telemetry.h"` and `telemetry_init()`.
- `CMakeLists.txt`: removed `"telemetry.c"` from SRCS.
- Added includes to `stream.c`: `bme280.h`, `esp_timer.h`, `esp_heap_caps.h`, `<math.h>`.
- Verified: scout_cam builds and works.
