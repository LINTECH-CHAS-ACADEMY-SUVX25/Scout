# fix/adapter-layers

Completed missing adapter layers so task files never include SDK headers directly. Verified: both projects build and run correctly.

## camera adapter (`scout_cam/components/camera/`)

- Added `camera_capture(const uint8_t **buf, size_t *len)` and `camera_release()` to `camera.h/c`
- `camera_fb_t` is now fully private; callers never see any ESP-IDF camera types
- `udp_stream.c` no longer includes `esp_camera.h`

## display adapter (`scout_screen/components/display/`)

- Removed `display_get_panel()` and `display_get_touch()` from public API
- Added `display_draw_bitmap(x1, y1, x2, y2, pixels)` and `display_read_touch(x, y)`
- `lvgl_port.c` no longer includes `esp_lcd_panel_ops.h` or `gt911.h`

## jpeg consolidation (`shared_components/jpeg/`)

- Moved `jpeg_decode` from `scout_screen/components/` to `shared_components/jpeg/`
- Renamed to `jpeg` to cover both decode and encode
- Added `jpeg_encode_rgb565()` using `espressif__esp_new_jpeg`
- Both projects now declare `espressif/esp_new_jpeg` in `idf_component.yml`
- `scout_screen/components/jpeg_decode/` deleted
