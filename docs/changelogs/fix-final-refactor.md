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
