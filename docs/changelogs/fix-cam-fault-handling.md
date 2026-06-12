# fix/cam-fault-handling

Crash observability + graduated camera recovery (HANDOFF.md Problem C). Based on
`feature/scene-state-machine`.

Three failure classes, three mechanisms — not conflated: hard crashes get core dump +
reset reason, hangs get the TWDT (already panics via `trigger_panic = true` in the
watchdog component — verified, no change needed), soft failures get reported upward
and the app decides.

## C1 — observability

- `shared_components/reset_info` — new component. `reset_info_log()` logs
  `esp_reset_reason()` at boot; abnormal reasons (panic, watchdog, brownout) log at
  WARN so they show under the cam's default WARN log level. Called first in the cam's
  `app_main`. Turns silent reboot loops into a visible story.
- `scout_cam/sdkconfig.defaults` — core dump to flash enabled (ELF format, CRC32),
  custom partition table. **Decode after a panic with `idf.py coredump-info`.**
- `scout_cam/partitions.csv` — new: default single-app layout + 64 KB `coredump`
  partition; factory grown 1 MB → 1.5 MB (app is ~0.9 MB, near the old limit). Fits
  in the configured 2 MB flash.
- **Build note:** the tracked `sdkconfig.defaults` only applies when `sdkconfig` is
  regenerated — run `rm sdkconfig && idf.py build` in `scout_cam/` (or set the new
  options via menuconfig). Flashing a new partition table requires `idf.py flash`
  (full), not `app-flash`.

## C2 — recovery policy (out of the driver)

- `components/camera` — `camera_init` returns `esp_err_t` instead of calling
  `esp_restart()`. The driver reports; it no longer decides to reboot the device
  (camera unplugged previously meant an invisible boot loop).
- `adapters/cam_state` — `cam_state_camera_start()` owns the policy: 3 attempts with
  0/500/2000 ms backoff (power-on glitches often succeed on attempt 2); on repeated
  failure sets the new `cam_status.camera_fault` and returns false — the node runs
  **degraded** instead of reboot-looping.
- `main.c` — `reset_info_log()` first, `camera_init()` call replaced by
  `cam_state_camera_start()`; flow stays linear, no branching in app_main.
- `stream.c` (cam) — degraded branch: with `camera_fault` set, skips capture but keeps
  draining motor commands (the RC link lives in this task) at a 20 ms cadence.
  Motors + telemetry stay alive without video.
