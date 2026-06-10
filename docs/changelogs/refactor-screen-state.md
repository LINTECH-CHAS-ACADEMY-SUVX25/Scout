# refactor/screen-state

## Added

- `adapters/ring_buffer.h/.c` — generic ring buffer utility (`ring_buf_t`, `ring_push`, `ring_avg`, `ring_min`, `ring_max`, `ring_fps_tenths`, `ring_snap`). No project dependencies.
- `adapters/screen_state.h/.c` — shared state hub for all cross-task data in scout_screen.
  - `screen_status_t` — `cam_connected`, `streaming`, `rssi`. Written by stream_run, read by render_run.
  - `screen_state_t` — snapshot struct returned by `screen_state_get`: render and stream loop times, decode/blit/lvgl/disp/transfer/rx_interval/frame_bytes stats, FPS values.
  - `screen_tick_t` — per-task timing context. Named `tick_slot_t` fields (`lvgl`, `decode`, `blit`, `transfer`, `bytes`). Each slot is self-describing: `ring`, `interval_ring`, `counts_frame`, `updates_streaming`, `prev_interval_ms`. `loop_ring` routes loop-time to a per-task ring.
  - `screen_state_tick(ctx)` — call at loop top; commits previous iteration's slot values to ring buffers and resets slots. Also pushes loop time to `ctx->loop_ring`.
  - `screen_state_tick_split(ctx, slot)` — records elapsed time since last split into a named slot.
  - `screen_state_render_tick_init(ctx)` / `screen_state_stream_tick_init(ctx)` — wire ring pointers and flags for each task before its loop starts.
  - `screen_state_is_streaming()` — returns true if a complete frame was received within the last 2 seconds. Replaces `frame_buf_is_streaming`.
- `adapters/cam_cmd.c/.h` — `cam_cmd_send_throttled(cmd)`: sends on command change, repeats every 200 ms as keepalive. Manages state and timer internally.

## Changed

- `adapters/frame_buf.c/.h` — stripped to pure ping-pong buffer management. Liveness tracking (`s_last_rx_ms`, `frame_buf_is_streaming`) removed; `frame_buf_publish` takes only `len`. No longer includes `esp_timer.h`.
- `render.c` — JPEG decode moved here from frame_buf. Timing via `screen_state_tick` / `screen_state_tick_split`. Command throttle replaced by `cam_cmd_send_throttled`. `esp_timer.h` and raw timer locals removed.
- `stream.c` — follows same tick pattern as render: `s_tick` context, `screen_state_stream_tick_init` in `stream_init`, `screen_state_tick` at loop top. On `FRAG_COMPLETE`, sets `s_tick.transfer` and `s_tick.bytes` slots directly — no explicit stat push. `esp_timer.h` removed.
- `monitor.c` — fixed stale `frame_buf_is_connected()` → `screen_state_is_streaming()`. Removed `frame_buf.h` include.
- `adapters/monitor_cmds.h/.c` — updated to `screen_state_t` layout. STREAM output now shows `loop` (last/avg ms) at the bottom of both Receive and Render sections. `STREAM_LINE_COUNT` updated to 14.

## Removed

- `stream_stats.h/.c` — replaced by `screen_state.h/.c`
- `stat_ring.h` — replaced by `ring_buffer.h/.c`
- `frame_buf_is_streaming` — replaced by `screen_state_is_streaming`
- All stat recording and timing logic from `frame_buf.c`
