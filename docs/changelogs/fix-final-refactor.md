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
