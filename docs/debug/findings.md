# Debugging Findings

Results from actual tool runs and from firmware debugging sessions during development.
Entries are grouped by tool.

---

## Host Simulator — Valgrind Memcheck

**Date:** 2026-06-15  
**Target:** `sim/` — UI code compiled natively against SDL2 on Linux  
**Command:**
```
SDL_VIDEODRIVER=offscreen SIM_SHOT=/tmp/sim_shot.bmp \
  valgrind --tool=memcheck \
           --leak-check=full \
           --show-leak-kinds=definite,indirect \
           --track-origins=yes \
           --error-exitcode=1 \
           ./sim
```

**Heap summary:**
```
total heap usage: 215,073 allocs, 211,776 frees, 133,768,071 bytes allocated
in use at exit:   298,643 bytes in 3,297 blocks
```

**Definite leaks found:**

| Size | Origin |
|---|---|
| 2,680 bytes | `sdl_init` → `SDL_CreateRenderer` → SDL2 renderer driver internals |
| 56 bytes (indirect) | `sdl_init` → `SDL_CreateRenderer` → SDL2 video driver |

Both stack traces lead entirely into SDL2 shared library code (`libSDL2-2.0.so`).
No frames from `scout_ui.c`, `main.c`, or any project source appear in the definite-leak
call stacks.

**Still reachable: 295,851 bytes (3,294 blocks)**  
All LVGL internal state — widget trees, style objects, label text buffers. LVGL holds
live pointers to all of these at exit; they are not leaked, LVGL simply does not call
`lv_deinit()` before process exit. This is expected and not a bug in our code.

**Verdict:** No memory leaks originating in project source code. The two definite-leak
blocks are a known SDL2 renderer driver issue on Linux (SDL2 2.30.0 / offscreen backend).

---

## Host Simulator — AddressSanitizer + UBSan

**Date:** 2026-06-15  
**Target:** `sim/` rebuilt with `-fsanitize=address,undefined -O1 -g`  
**Build command:**
```
make CFLAGS="-O1 -g -fsanitize=address,undefined -Wall -I. \
             -I../scout_screen/components/lvgl_port/lvgl__lvgl \
             -I../shared_components/rc_protocol \
             -I../scout_screen/components/ui \
             -I../scout_screen/components/cam_diag_fmt \
             -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE \
             $(sdl2-config --cflags)" \
     LDFLAGS="$(sdl2-config --libs) -lm -fsanitize=address,undefined"
```

**Run command:**
```
ASAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  SDL_VIDEODRIVER=offscreen SIM_SHOT=/tmp/sim_asan.bmp SIM_SHOT_MS=4000 ./sim
```

The `SIM_SHOT_MS=4000` flag runs 4 000 ms of simulated LVGL time, covering the full
intro sequence (4 steps × 1 000 ms each) before taking the screenshot and exiting.

**No ASan errors detected** — no heap buffer overflows, no stack buffer overflows,
no use-after-free, no use of uninitialised memory in project code.

**No UBSan errors detected** — no integer overflow, signed overflow, misaligned access,
or null pointer dereference in project code.

**LeakSanitizer summary:**
```
SUMMARY: AddressSanitizer: 2920 byte(s) leaked in 4 allocation(s)
```

Leak breakdown:

| Bytes | Kind | Stack trace top |
|---|---|---|
| 2,680 | Direct | `sdl_init` → `SDL_CreateRenderer` → SDL2 internal |
| 128 | Direct | `sdl_init` → `SDL_CreateRenderer` → libpthread once-init → SDL2 audio/video driver |
| 56 | Indirect | `flush_cb` (main.c:36) → `SDL_UpdateTexture` → SDL2 → libxml2 |
| 56 | Indirect | same as above (second texture upload) |

All four allocations trace to SDL2 or its dependencies (libxml2, libpthread). The entry
point `flush_cb` is our code (`main.c:36`), but the leak itself is inside SDL2's texture
upload path — SDL2 lazily initialises a font/renderer subsystem on the first
`SDL_UpdateTexture` call and does not release it on `SDL_Quit()` in offscreen mode.

**Verdict:** Zero errors in project application code. All LeakSanitizer findings are SDL2
library behaviour, not bugs in `scout_ui.c` or `main.c`.

---

## Firmware — Task Watchdog False Trigger During WiFi Scan

**Branch:** `fix/watchdog-cam-recovery`  
**Tool:** `idf.py monitor` (serial panic decoder, 115 200 baud)

**Symptom:** Scout-CAM rebooted after a dirty reset with:

```
Guru Meditation Error: Core 0 panic'ed (TG1WDT_SYS_RESET)
Task watchdog got triggered. The following tasks did not reset the watchdog in time:
 - IDLE0 (CPU 0)
```

**Investigation:** The reset reason `TG1WDT_SYS_RESET` and the starved task (`IDLE0`)
were decoded automatically by `idf.py monitor` from the ELF symbol table. The watchdog
was configured with `idle_core_mask = 0x3` (both cores).

ESP-IDF's WiFi driver runs at priority 23. During a full 14-channel scan it holds CPU 0
long enough to starve the idle task past the 5 000 ms watchdog threshold. This only
occurred after an unclean reset (normal cold boot completes the scan fast enough).

**Root cause:** Watching idle tasks on all cores with the default timeout is incompatible
with the ESP32-CAM's WiFi scan behaviour after dirty reset.

**Fix:** `watchdog_init()` was given an optional `watchdog_config_t *`. Scout-CAM now
passes `idle_core_mask = 0` to exclude idle tasks from watchdog supervision:

```c
// scout_cam/main/main.c
watchdog_config_t wtd_cfg = { .timeout_ms = 5000, .idle_core_mask = 0 };
watchdog_init(&wtd_cfg);
```

---

## Firmware — Camera Init Hang After Dirty Reset (I2C Bus Lock)

**Branch:** `fix/watchdog-cam-recovery`  
**Tool:** `idf.py monitor` boot log analysis

**Symptom:** After a dirty reset, Scout-CAM occasionally stopped at:

```
I (312) camera: Initialising...
```

No further output. Required manual power-cycle to recover.

**Investigation:** A log line added immediately after `esp_camera_init()` never appeared,
confirming the function blocked indefinitely. The I2C bus used by the OV2640 sensor can
be left mid-transaction after an unclean reset; the camera driver stalls waiting for an
ACK from the sensor that never arrives. The original code used `ESP_ERROR_CHECK()` which
calls `abort()` on error — but when the driver hangs, `abort()` is never reached.

**Root cause:** I2C bus locked after dirty reset. No recovery path in the camera init
sequence.

**Fix:** Replaced `ESP_ERROR_CHECK` with an explicit check that calls `esp_restart()`,
releasing the I2C bus through the hardware reset sequence:

```c
// scout_cam/components/camera/camera.c
esp_err_t err = esp_camera_init(&config);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "camera init failed: %s — restarting", esp_err_to_name(err));
    esp_restart();
}
```

---

## Firmware — Display FPS Reported Above Theoretical Maximum (MÅSTE REDIGERAS, DUBBELKOLLAS)

**Branch:** `fix/render_task_issues`  
**Tool:** UART diagnostic console (`STREAM` command)

**Symptom:** `STREAM` output showed `fps` consistently above `max fps`:

```
=== STREAM ===
Receive
  max fps     30.3fps    29.4fps
Render
  fps         58.7fps    55.1fps
```

A display rate higher than the receive rate is physically impossible — each rendered
frame must originate from a received frame.

**Investigation:** Live `STREAM` stats were captured over several seconds. The display
fps was approximately double the receive rate, which pointed to double-counting rather
than a genuine anomaly. Code review of `render.c` found that `frame_buf_record_disp_frame()`
was called on every loop iteration, not only when a new frame was decoded.

**Root cause:** The FPS counter call was outside the `if (new_frame)` branch, so idle
loop iterations — where no new frame arrived — were counted as displayed frames,
inflating `disp_fps` to approximately the render loop rate.

Also found: the camera region was re-blitted (~19 ms, 460 KB `memcpy`) on every loop
even when no new frame had arrived. Since LVGL uses partial refresh (`full_refresh=0`),
the last frame persists in the framebuffer; the re-blit was pure wasted time.

**Fix:** Moved both the FPS record call and the blit inside the new-frame branch:

```c
if (frame_buf_try_decode(&frame)) {
    display_blit_region(...);
    frame_buf_record_disp_frame();
}
```

---

## Firmware — LEDC Peripheral Clock Conflict Causing Camera Boot Loop

**Branch:** `fix/analog-joystick`  
**Tool:** `idf.py monitor` (serial boot log)

**Symptom:** After adding PWM motor speed control, Scout-CAM entered an infinite reboot
loop immediately after boot. The serial log showed the camera init failing on every attempt:

```
I (312) camera: Initialising...
E (890) camera: init failed: ESP_ERR_INVALID_STATE
I (891) cam_state: camera init attempt 1/3 failed
...
I (4391) cam_state: camera init attempt 3/3 failed
```

**Investigation:** `idf.py monitor` made the loop visible immediately — without the monitor
the device would have appeared to silently malfunction. The error `ESP_ERR_INVALID_STATE`
from `esp_camera_init` pointed to a peripheral already configured in an incompatible state.

The camera driver (OV2640 XCLK) uses `LEDC_HIGH_SPEED_MODE` + `LEDC_TIMER_0` +
`LEDC_CHANNEL_0` to generate a 24 MHz clock signal. The newly added motor PWM code also
initialised in `LEDC_HIGH_SPEED_MODE`. High-speed LEDC timers share a single peripheral
clock source register. When the motor initialised first, it configured the global clock;
when `esp_camera_init()` later tried to configure the same timer, it found the peripheral
in a conflicting state and returned `ESP_ERR_INVALID_STATE`.

**Root cause:** Both motor PWM and the camera driver used `LEDC_HIGH_SPEED_MODE`, which
shares hardware between all channels in that mode. The motor init ran before the camera
init, leaving the shared clock register in a state incompatible with the camera driver.

**Fix:** Motor PWM switched to `LEDC_LOW_SPEED_MODE`, which has completely separate
hardware (independent timers, separate clock registers):

```c
// scout_cam/components/l298n/l298n.c
#define ENA_SPEED_MODE  LEDC_LOW_SPEED_MODE   // must not share with camera's HIGH_SPEED_MODE
#define ENA_TIMER       LEDC_TIMER_1
#define ENA_CHANNEL     LEDC_CHANNEL_1
```

After the fix, `idf.py monitor` confirmed a clean boot with no camera init error.
The low-speed timer operates identically for 1 kHz motor PWM — there is no functional
difference for this use case.

**Currently in code:** `scout_cam/components/l298n/l298n.c:16`

---

## Firmware — Boot Loop Observability via reset_info and Core Dump

**Branch:** `fix/cam-fault-handling`  
**Tool:** `idf.py monitor`, `idf.py coredump-info`

**Problem:** Before this change, abnormal reboots (panic, watchdog, brownout) were silent.
The device would restart and log `I (0) reset_info: reset reason: power-on` even after a
panic, because the reset reason was never checked. A boot loop looked identical to normal
startup in the serial output.

**Solution — reset_info component:**

`shared_components/reset_info/reset_info.c` reads `esp_reset_reason()` at boot and
logs abnormal reasons at `WARN` level so they are visible even with the default log
level filter:

```c
void reset_info_log(void)
{
    esp_reset_reason_t r = esp_reset_reason();
    bool abnormal = r == ESP_RST_PANIC || r == ESP_RST_INT_WDT ||
                    r == ESP_RST_TASK_WDT || r == ESP_RST_WDT || r == ESP_RST_BROWNOUT;
    if(abnormal)
        ESP_LOGW(TAG, "reset reason: %s (%d)", reason_name(r), r);
    else
        ESP_LOGI(TAG, "reset reason: %s (%d)", reason_name(r), r);
}
```

Called first in `scout_cam/main/main.c` before any task is started. A reboot loop now
shows a repeating `W (0) reset_info: reset reason: task watchdog (8)` instead of a
silent stream of startup messages.

**Solution — core dump to flash:**

`scout_cam/sdkconfig.defaults` enables core dump storage to a dedicated 64 KB partition:

```
CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=y
CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=y
CONFIG_ESP_COREDUMP_CHECKSUM_CRC32=y
```

After a panic the full register state, task stacks, and heap snapshot are written to
flash. Decoded post-mortem without a JTAG probe:

```bash
idf.py coredump-info -p /dev/ttyUSB0
idf.py coredump-debug -p /dev/ttyUSB0   # full GDB session against the dump
```

**Solution — graduated camera recovery:**

`cam_state_camera_start()` retries camera init three times with increasing backoff
(0 / 500 / 2 000 ms) before giving up. Power-on glitches that leave the I2C bus briefly
unstable typically clear by the second attempt:

```c
// scout_cam/main/adapters/cam_state.c
static const uint16_t s_camera_backoff_ms[CAMERA_INIT_ATTEMPTS] = { 0, 500, 2000 };

bool cam_state_camera_start(void)
{
    for(int i = 0; i < CAMERA_INIT_ATTEMPTS; i++) {
        if(s_camera_backoff_ms[i]) vTaskDelay(pdMS_TO_TICKS(s_camera_backoff_ms[i]));
        if(camera_init() == ESP_OK) return true;
        ESP_LOGW(TAG, "camera init attempt %d/%d failed", i + 1, CAMERA_INIT_ATTEMPTS);
    }
    ...
}
```

**Currently in code:** `shared_components/reset_info/reset_info.c`,
`scout_cam/sdkconfig.defaults`, `scout_cam/partitions.csv`,
`scout_cam/main/adapters/cam_state.c:21`

