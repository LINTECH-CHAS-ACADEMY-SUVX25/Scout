# refactor/screen-state

## Added

- `adapters/ring_buffer.h/.c` — generic ring buffer utility (`ring_buf_t`, `ring_push`, `ring_avg`, `ring_min`, `ring_max`, `ring_fps_tenths`, `ring_snap`). No project dependencies.
- `adapters/screen_state.h/.c` — single adapter owning all rolling performance statistics and connection state for scout_screen.
  - `screen_status_t` — `cam_connected` (WiFi, set by stream.c) and `streaming` (frame liveness, set by stream.c)
  - `screen_state_t` — snapshot struct with `loop`, `decode`, `blit`, `lvgl`, `disp`, `transfer`, `rx_interval`, `frame_bytes` stats and FPS values
  - `screen_tick_t` — per-task timing context with named `tick_slot_t` fields (`lvgl`, `decode`, `blit`, `transfer`). Each slot carries its target ring buffer pointer and an `is_frame` flag.
  - `screen_state_tick(ctx)` — call at the top of any task loop; commits the previous iteration's splits and starts a fresh timer. Also records full loop time tick-to-tick.
  - `screen_state_tick_split(ctx, slot)` — records elapsed time since the previous split into the named slot. Pass `&ctx->lvgl`, `&ctx->decode`, etc.
  - `screen_state_render_tick_init(ctx)` — wires ring pointers and `is_frame` flag for the render task context.
  - `screen_state_push_transfer(now_ms, len, transfer_ms)` — called by stream.c after a complete frame is received.

## Changed

- `adapters/frame_buf.c/.h` — removed all stat recording, timing, and logging. Now only manages ping-pong buffer handoff. `frame_buf_publish` no longer records stats; callers are responsible. `frame_buf_is_streaming` retains its internal liveness check.
- `render.c` — JPEG decode moved here from frame_buf. Timing replaced by `screen_state_tick` / `screen_state_tick_split` stopwatch API. `s_tick` context declared at file scope; `screen_state_render_tick_init` called in `render_init`.
- `stream.c` — updates `screen_status.cam_connected` and `screen_status.streaming` each loop iteration. Calls `screen_state_push_transfer` after a complete frame is assembled.
- `adapters/monitor_cmds.h/.c` — updated from removed `stream_stats_t` to `screen_state_t`; field access updated to match new struct layout.
- `CMakeLists.txt` — added `ring_buffer.c` and `screen_state.c` to SRCS; removed deleted stat files.

## Removed

- `stream_stats.h/.c` — replaced by `screen_state.h/.c`
- `stat_ring.h` — replaced by `ring_buffer.h/.c`
- All stat recording and timing logic from `frame_buf.c`
