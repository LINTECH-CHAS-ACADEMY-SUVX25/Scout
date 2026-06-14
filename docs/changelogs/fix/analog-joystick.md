# Changelog: fix/analog-joystick

## Summary
Tasks 2a and 3: Joystick x/y analog values exposed; motor control via proportional PWM.

Screen-side joystick now returns raw x/y coordinates (-255..255) instead of binary CMD bytes. The camera receives these analog values over UDP and converts them to direction bitmask + speed via `joy_to_motor()` — full ownership of motor policy (deadzone, speed curve) now lives on the cam side. PWM via LEDC on GPIO 1 (ENA/ENB tied together).

## Files Changed

### Protocol Layer

**`shared_components/rc_protocol/rc_protocol.h`**
- Added `joy_pkt_t` struct (4-byte packed): `int16_t x, y` (-255..255)
- CMD_* byte defines remain but now cam-internal only (screen no longer sends them)

### Screen-Side UI

**`scout_screen/components/ui/scout_ui.h`**
- Replaced `uint8_t scout_ui_get_cmd(void)` with `void scout_ui_get_joy(int16_t *x, int16_t *y)`

**`scout_screen/components/ui/scout_ui.c`**
- Changed state: `static volatile int16_t s_joy_x, s_joy_y` (was `s_cmd`)
- `joy_event()` PRESSING: scale joystick pixel displacement (±JOY_RADIUS px) to ±255 per axis; store in `s_joy_x/y`
- `joy_event()` RELEASED/PRESS_LOST: zero out `s_joy_x/y`
- Badge display (CMD_* UI) uses local `cmd` variable computed from deadzone (±15px ≈ ±112 units) — identical visual feedback to before
- New `scout_ui_get_joy()` returns current joystick position

**`scout_screen/main/render.c`**
- Removed temporary `joy_to_cmd()` shim (was in task 2a, now cam-owned)
- Read `scout_ui_get_joy()` and call `cam_cmd_send_throttled(jx, jy)` directly

**`scout_screen/main/adapters/cam_cmd.h`**
- `void cam_cmd_send(int16_t x, int16_t y)`
- `void cam_cmd_send_throttled(int16_t x, int16_t y)`

**`scout_screen/main/adapters/cam_cmd.c`**
- `cam_cmd_send()`: pack x/y into `joy_pkt_t`, send 4 bytes via UDP
- `cam_cmd_send_throttled()`: send on axis delta >5 units OR 200ms keepalive (no change in throttling policy, just different data shape)
- Added `#include <stdlib.h>` for `abs()`

### Camera-Side Motor Control

**`scout_cam/main/adapters/motor_cmd.h`**
- `void motor_cmd_send(int16_t x, int16_t y)`
- `bool motor_cmd_recv(int16_t *x, int16_t *y, uint32_t timeout_ms)`

**`scout_cam/main/adapters/motor_cmd.c`**
- FreeRTOS queue element size changed from `sizeof(uint8_t)` to `sizeof(joy_pkt_t)` (4 bytes)
- Receive path unpacks `joy_pkt_t` into x/y return values

**`scout_cam/main/adapters/cam_state.c`**
- `cam_state_process_cmds()` and `cam_state_try_resume()`: receive `joy_pkt_t` (4 bytes) from UDP socket
- Forward `pkt.x, pkt.y` to `motor_cmd_send()`

**`scout_cam/main/motor.c`**
- New `joy_to_motor(int16_t x, int16_t y, uint8_t *cmd, uint8_t *speed)`:
  - Deadzone: ±112 units (44% of full deflection)
  - Speed = `sqrt(x² + y²)` capped at 255
  - Direction bitmask (CMD_FORWARD/BACKWARD/LEFT/RIGHT) set independently; stop if within deadzone
  - Conflicting directions (forward+backward or left+right) cancel in hardware
- `motor_run()`: receive x/y via `motor_cmd_recv()`, unpack via `joy_to_motor()`, call `l298n_apply(cmd, speed)`
- 500ms timeout safety unchanged
- Added `#include <math.h>` for `sqrtf()`

**`scout_cam/components/l298n/l298n.h`**
- `void l298n_apply(uint8_t cmd, uint8_t speed)` — speed param added (0-255 duty cycle)

**`scout_cam/components/l298n/l298n.c`**
- Direction GPIO config unchanged (IN1-IN4)
- Added LEDC PWM setup:
  - `#define PIN_ENA GPIO_NUM_1` (ENA/ENB tied together)
  - `#define ENA_SPEED_MODE LEDC_LOW_SPEED_MODE` (critical: separate hardware from camera's HIGH_SPEED_MODE to avoid peripheral clock conflict)
  - `#define ENA_TIMER LEDC_TIMER_1` and `#define ENA_CHANNEL LEDC_CHANNEL_1`
  - Timer config: 8-bit resolution, 1 kHz frequency, auto clock selection
  - Channel config: GPIO 1, starts at duty=0
- `l298n_apply()`: calls `ledc_set_duty()` and `ledc_update_duty()` before updating direction pins

**`scout_cam/components/l298n/CMakeLists.txt`**
- Added `esp_driver_ledc` to REQUIRES

**`scout_cam/main/stream.c`** (minor)
- `motor_cmd_send(0, 0)` call updated to new 2-argument signature on camera capture failure

## Technical Notes

### LEDC Mode Selection

The camera driver uses `LEDC_HIGH_SPEED_MODE` + TIMER_0 + CHANNEL_0 (XCLK @ 24 MHz). High-speed LEDC timers share a single peripheral clock source register. When our motor PWM was configured in HIGH_SPEED_MODE first, it set the global clock. When `esp_camera_init()` later tried to configure TIMER_0, the second clock config conflicted → `esp_camera_init` failed → `esp_restart()` → reboot loop.

**Solution:** Motor LEDC uses `LEDC_LOW_SPEED_MODE`, which has completely separate hardware (independent timers, separate clock registers). Zero overlap with camera.

### Joystick Scaling

Screen-side: 640×480 display, joystick centered in video region. Displacement from center is [0, JOY_RADIUS] px (JOY_RADIUS=34). Scaled to [-255, 255] per axis: `(int16_t)((dx * 255) / JOY_RADIUS)`. Positive y = forward, positive x = right.

Cam-side: Deadzone ±112 units (44% of ±255). Full deflection (±255) maps to speed 255 (magnitude of vector). Diagonal deflection (x=y=180) yields speed ~254.6 → capped at 255.

## Device Verification

- Joystick center position (within deadzone) → motors stop (speed=0)
- Gentle deflection → proportional speed reduction
- Full deflection → maximum speed (255)
- All diagonal directions (FwdLft, FwdRgt, BwdLft, BwdRgt) tested
- GPIO 1 LEDC active after `l298n_init()` → screen-side serial logging stops (expected, user aware)
- Cam boots cleanly, no reboot loop

## Build Instructions

Both nodes must be rebuilt (protocol changed from 1-byte to 4-byte format):

```bash
cd scout_cam
idf.py build
idf.py -p /dev/ttyUSB0 flash

cd scout_screen
idf.py build
idf.py -p /dev/ttyUSB1 flash
```

Cam serial logging disabled after flash (GPIO 1 taken by LEDC). Screen-side monitor works normally.
