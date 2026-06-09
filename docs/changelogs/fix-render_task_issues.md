# fix/render_task_issues

Fixes to the render loop's display-frame handling and stats.

## Display fps counted per loop instead of per frame

`render.c` latched `has_frame` and then re-blitted + called `frame_buf_record_disp_frame()`
every loop iteration, so `disp_fps` measured the loop rate — its "last" value spiked to
~2× and the average drifted above the frame arrival rate (`max fps`).

- `frame_buf_record_disp_frame()` is now called only when a new frame was actually decoded.

## Redundant per-loop re-blit removed

The camera region was re-blitted (~19 ms, 460 KB memcpy) on every loop even when no new
frame had arrived. LVGL renders in partial mode (`full_refresh=0`) and only redraws its
dirty areas (joystick / connection indicator), which don't overlap the camera region, so
the last frame persists in the framebuffer between decodes.

- `render.c` now blits only when `frame_buf_try_decode()` returns a new frame; the
  `has_frame` latch is gone.
- Verified on device: image stays flicker-free (confirms the re-blit was redundant and the
  `num_fbs=2` panel isn't swapping destructively), and average display fps increased since
  idle loops no longer spend ~19 ms blitting.
