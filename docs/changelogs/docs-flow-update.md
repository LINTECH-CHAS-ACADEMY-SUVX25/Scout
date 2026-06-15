# Changelog: docs/scout_cam_flow.md + docs/scout_screen_flow.md update

Updated both flow docs to reflect the current codebase after tasks 1–9.

## scout_cam_flow.md

- Startup sequence now includes `reset_info_log`, `watchdog_init`, `cam_state_camera_start`,
  `bme280_init`, and `telemetry_init`
- Added `telemetry_run` task row and full dependency edges
- Added `cam_state`, `bme280`, `reset_info` nodes to the Mermaid graph
- `stream_run` table: added `cam_state` adapter entry
- `motor_run` table: updated `motor_cmd_recv` to `joy_pkt_t` (not CMD byte);
  `l298n_apply` now takes direction + speed
- Added `telemetry_run` per-task section
- Adapter table: added `cam_state`, `bme280`, `reset_info`; updated `motor_cmd` and `l298n`
  for the PWM/joy_pkt_t protocol; removed stale "no UART monitor" note

## scout_screen_flow.md

- Startup sequence now includes `watchdog_init`, `scout_ui_init` (intro bar), and the
  intro step calls between subsystem inits
- Added `cam_diag_run` task row with design note (spawned by `monitor_init`, belongs in `main.c`)
- Added `screen_state`, `scene`, `ring_buffer`, `cam_diag`, `cam_diag_fmt`, `scout_ui` nodes
  to the Mermaid graph
- `stream_run` table: added `screen_state` and `wifi_ap` entries; clarified camera IP learning via `cam_cmd`
- `render_run` table: added `screen_state`, `scene`, `scout_ui`, `cam_diag_fmt`; `cam_cmd`
  now sends `joy_pkt_t` via `cam_cmd_send_throttled`
- `monitor_run` table: added `screen_state`; noted live STREAM mode in `monitor_cmds`
- Added `cam_diag_run` per-task section
- Adapter table: added `screen_state`, `scene`, `ring_buffer`, `cam_diag`, `cam_diag_fmt`,
  `scout_ui`; updated `cam_cmd` for `joy_pkt_t`; added `uart_console_try_getchar` note
