# feature/scene-state-machine

Uniform app-state handling for scout_screen (HANDOFF.md Problem B). One way to set UI
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
