# feature/add-custom-resolution

Crop the camera stream from 640×480 to a centered 480×480 square using OV2640 sensor
windowing, so the screen (the render bottleneck) decodes and blits less per frame.

## Why

The screen is the pipeline bottleneck (~13 fps display; decode ~38 ms + blit ~24 ms),
while the cam runs at 25 fps. A 480×480 frame is ~75% of the pixels, so the screen renders
faster. The crop is done at the sensor (no per-frame decode/encode on the cam), so the cam
stays at 25 fps and overall FPS improves rather than regressing.

## Changes

### `shared_components/rc_protocol/rc_protocol.h`
- `CAM_W` 640 → 480 (CAM_H stays 480). Single source of truth — the screen's buffer
  sizes, blit region, and centering all derive from these and adapt automatically.

### `scout_cam/components/camera/camera.c`
- After `esp_camera_init`, `camera_apply_crop()` calls `set_res_raw` on the OV2640 to
  output a centered CAM_W×CAM_H window.
- Windowing math: VGA is produced in SVGA mode from an 800×600 sensor window (offset 0,0)
  scaled 0.8× to 640×480. To crop the centered 480×480 at the same scale: horizontal
  window 600 wide centered in 800 (offset_x = 100), vertical unchanged (600 high, offset 0),
  output 480×480. Crop constants are local `#define`s with the derivation; output dims
  reference `CAM_W`/`CAM_H` (no duplicated literal).
- On windowing failure the sensor is left at VGA and a warning is logged — a crop error
  must not stop the camera from working.
- `#include "rc_protocol.h"` added for `CAM_W`/`CAM_H`.

### `scout_cam/components/camera/CMakeLists.txt`
- Added `rc_protocol` to `REQUIRES`.

### scout_screen
- No code change. All dimension-dependent code is driven by `CAM_W`/`CAM_H`; the camera
  region recenters to (272, 60) and buffers shrink automatically.

## Measurements (on device)

The smaller frame means less for the screen to decode and blit each frame:

| Metric | Before (640×480) | After (480×480) |
|---|---|---|
| frame size | ~10.3 KB | ~8.4 KB |
| decode | ~38 ms | ~29 ms |
| blit | ~24 ms | ~19 ms |
