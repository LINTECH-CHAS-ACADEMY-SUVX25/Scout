# Stream performance improvements

Measured baseline (before): frame gap 39 ms avg (~25 fps camera), render fps avg 15.0 fps.
Measured result (after):     frame gap 33 ms avg (~30 fps camera), render fps avg 18.0 fps.

## scout_cam

### camera.c
- `xclk_freq_hz` raised from 20 MHz to 24 MHz. OV2640 captures faster, reducing frame gap
  from ~39 ms to ~33 ms (camera output ~25 fps → ~30 fps).

### sdkconfig.defaults
- `CONFIG_FREERTOS_HZ=1000` — 1 ms tick granularity. Required for `vTaskDelay(1)` in the
  render loop to yield for 1 ms rather than 10 ms.
- `CONFIG_ESPTOOLPY_FLASHMODE_QIO=y` + `CONFIG_ESPTOOLPY_FLASHFREQ_80M=y` — flash upgraded
  from DIO 40 MHz to QIO 80 MHz for faster instruction fetches.
- `CONFIG_SPIRAM_SPEED_80M=y` — PSRAM raised from 40 MHz to 80 MHz, doubling bandwidth for
  frame buffer reads and JPEG decode output writes.
- `CONFIG_LOG_DEFAULT_LEVEL_WARN=y` / `CONFIG_LOG_DEFAULT_LEVEL=2` — reduces serial log
  overhead in the hot path (was INFO).

## scout_screen

### render.c
- `vTaskDelay(pdMS_TO_TICKS(10))` replaced with `vTaskDelay(connected ? 1 : pdMS_TO_TICKS(20))`.
  Removes the 10 ms floor from the render cycle when connected (decode+blit was the real
  bottleneck). Falls back to 20 ms when disconnected to prevent idle task starvation.
- LVGL timing now accumulates across loop iterations and is recorded once per decoded frame,
  keeping the LVGL avg stat on the same cadence as decode and blit. Previously, the tight
  loop flooded the ring buffer with 0 ms samples, masking real LVGL work.

### rgb_lcd_port.h
- `EXAMPLE_LCD_PIXEL_CLOCK_HZ` raised from 12 MHz to 16 MHz. Eliminates visible panel
  flicker (12 MHz gave ~13 fps display refresh, below the threshold for this panel).

### rgb_lcd_port.c
- `clk_src` changed from `LCD_CLK_SRC_DEFAULT` (PLL_F160M, 160 MHz) to `LCD_CLK_SRC_PLL240M`
  (240 MHz). 240 / 15 = 16 MHz exactly, avoiding the clock jitter from a non-integer
  division that PLL_F160M would produce at 16 MHz.

### sdkconfig.defaults
- `CONFIG_FREERTOS_HZ=1000` — same reason as cam side.
- `CONFIG_LWIP_UDP_RECVMBOX_SIZE=16` — UDP receive mailbox increased from 6 to 16. A ~6 KB
  frame arrives as ~5 fragments; the default of 6 leaves minimal headroom if WiFi delivers
  a burst faster than the stream task drains the socket, causing fragment drops and lost frames.

## Investigated, not applied

- LCD blanking values reduced to hsync/vsync pulse=4, back/front porch=8 (from a third-party
  example). Caused the panel to display only a 20 px wide strip — the panel requires the
  original Waveshare values (hsync_pulse_width=162, back=152, front=48). Reverted.
- PSRAM speed (80 MHz OPI) was already in place on screen side — not a remaining lever.
- Hardware JPEG decode: not available on ESP32-S3 (only ESP32-P4). Software decoder
  (`esp_new_jpeg`) is already the fastest option for this chip.
- `esp_new_jpeg` output stride: not supported by the decoder API. Direct decode into the
  LCD framebuffer is not possible without a stride parameter; the separate blit step is
  unavoidable with the current decoder.
