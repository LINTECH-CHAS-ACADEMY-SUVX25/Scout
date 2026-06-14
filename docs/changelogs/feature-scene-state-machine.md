# feature/scene-state-machine

Uniform app-state handling for scout_screen. One way to set UI
state from any task/core (`screen_state_set_scene`), one place that reacts to it on the
LVGL core (`scene_render`).

## Scene state machine

**Modes vs values:** scenes are mutually exclusive UI modes (an FSM); telemetry values
stay out of it (field table, task 4). The two are never merged.

### Layer 1 — uniform setter (`adapters/screen_state.h/.c`)

- `scene_t` — `SCENE_BOOTING / WAITING / STREAMING / DISCONNECTED`
  (+ `SCENE_COUNT` sentinel as table bound).
- `screen_state_set_scene(s)` — callable from any task/core; lock-free single-word
  store, last writer wins. Logs each transition once (`scene waiting -> streaming`),
  which doubles as the on-device verification trail.
- `screen_state_get_scene()`, `screen_state_scene_name(s)`.
- `screen_state_has_streamed()` — true once any full frame has arrived since boot;
  distinguishes WAITING (never streamed) from DISCONNECTED (stream died).

### Layer 2 — render-owned reaction (`adapters/scene.c/.h`)

- `scene_render()` — called once per render tick; the single centralized edge-detect.
  Declarative `scene_cfg_t` table maps each scene to its UI reaction
  (`cam_connected` indicator + `overlay_text`, NULL = hide). Adding a scene = one
  enum value + one table row + the sites that set it. All LVGL stays on core 1.
- `lvgl_port_overlay(text)` — shows `text` on a black overlay covering the camera
  region, hides it when NULL. Overlay widget created hidden in `lvgl_port_ui_init`.
  Removed a stale comment referencing the deleted `ui.c`.

### Task wiring

- `render.c` — `was_connected` edge-detection dropped; loop starts with `scene_render()`.
  Blit gated on `streaming` so a buffered frame can't overwrite the overlay (the blit
  bypasses LVGL, which would otherwise not know to redraw it).
- `stream.c` — sets the scene directly each iteration from what it observes
  (per the agreed scene-authority decision: tasks set scenes, last writer wins):
  never streamed → WAITING; live → STREAMING; cam gone → DISCONNECTED.
- `monitor` STATUS command now prints the active scene (`scene  streaming`).

## Signal-lost input layer (merged from `fix/signal-lost`, 2026-06-14)

The FSM defined `SCENE_DISCONNECTED` but it could never fire: `stream_run` blocked in `udp_rx`
when the cam went silent, so the liveness re-check never ran. Merged the input layer only:

- `shared_components/udp/udp.h/.c` — `udp_set_recv_timeout(sock, seconds)` (`SO_RCVTIMEO`),
  mirroring `udp_set_send_timeout`.
- `stream.c` — `udp_set_recv_timeout(sock, 1)` on the video socket + `if(n <= 0) continue;`
  after the status/scene update. The loop now iterates ~1×/s, re-evaluates
  `screen_state_is_streaming()`, and sets `SCENE_DISCONNECTED` ("CAM DISCONNECTED") when frames
  stop. Reassembly is skipped on a timed-out recv.

## Video-region lock (2026-06-14)

The themes dropdown rebuilds the whole UI, which invalidated the full screen and let the LVGL
flush paint over the live video (the camera region is owned by the direct `display_blit_region`,
not LVGL).

- `lvgl_port.h/.c` — `lvgl_port_set_video_region(x,y,w,h)` + `lvgl_port_video_lock(bool)`.
  `flush_cb` clips the registered camera rectangle out of the flushed band while the lock is on
  (rows above/below drawn whole; overlapping rows draw only the left/right segments).
- `render.c` — registers the region at init; sets the lock = `screen_status.streaming` each tick
  (same signal as the blit gate). Nothing draws over the video during `SCENE_STREAMING`; the lock
  is off otherwise so the scene overlay repaints the region in WAITING/DISCONNECTED.
