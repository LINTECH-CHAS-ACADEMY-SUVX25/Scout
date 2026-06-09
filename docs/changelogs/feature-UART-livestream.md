# feature/UART-livestream

Live STREAM mode for the UART monitor: instead of a one-shot stats dump, `STREAM` now
starts a refreshing live view with two flat sections (Receive / Render), each with a
`last / avg` column header, that redraws in place every 500 ms and exits on `q`.

## Changes

### `uart_console` (component)

- Added `uart_console_try_getchar()`: non-blocking read, returns the byte value or -1 if
  nothing is available (timeout 0)

### `frame_buf` (adapter)

- `stream_stats_t` gained `last_interval_ms` / `avg_interval_ms` (wall-clock gap between
  received frames) and `last_disp_interval_ms` (gap between displayed frames)
- `frame_buf_publish` now records `s_last_interval_ms` and `s_avg_interval_ms` alongside
  the existing fps ring; `frame_buf_record_disp_frame` records `s_last_disp_interval_ms`
- These back the new "frame gap" / "max fps" / "fps" rows without recomputing in the UI

### `monitor_cmds` (adapter)

- `cmd_stream` renamed to `monitor_cmd_stream` and made public; renders two flat
  sections (`STREAM_LINE_COUNT 12` lines total, no blank separators)
- Receive section: frames, frame size, transfer, frame gap, max fps
- Render section: lvgl, decode, blit, fps
- Each section has a `last / avg` column header instead of repeating "avg" per row
- "max fps" = 1000 / frame gap (the ceiling the display rate can't exceed); avg taken
  from `rx_fps_tenths`. Render "fps" avg taken from `disp_fps_tenths`
- Layout helpers added: `stat_header` (section name + last/avg headers), `stat_row`
  (label + last + avg), `stat_one` (single value), `fmt_fps`
- `STREAM_LINE_COUNT 12` in `monitor_cmds.h` so `cmd_stream_live` builds the correct
  cursor-up escape without a magic number
- `cmd_stream_live` private helper: prints stats, then every 500 ms checks
  `uart_console_try_getchar()` and either moves the cursor up `STREAM_LINE_COUNT` lines
  (`\033[12A`), clears to end of screen (`\033[J`), and reprints, or exits on `q`
- `monitor_dispatch` routes STREAM to `cmd_stream_live`; `stream` parameter removed
  (live loop fetches stats directly via `frame_buf_get_stats`)
- HELP text updated: "STREAM  live stream stats (q to exit)"

### `monitor` (task)

- No logic change — `monitor_run` calls `monitor_dispatch` as before; all STREAM
  behaviour lives in the adapter per `TASK_TEMPLATE.md`
