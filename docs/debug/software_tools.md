# Debugging Tools

## 1. GDB via OpenOCD (JTAG — firmware)

ESP-IDF ships OpenOCD support for JTAG debugging of both ESP32 targets. The debug probe
connects to the four JTAG pins on the board and gives full stop-mode debugging: breakpoints,
watchpoints, register inspection, and stack unwinding.

**Start a debug session:**

```bash
# Terminal 1 — OpenOCD server
idf.py -p /dev/ttyUSB0 openocd

# Terminal 2 — GDB client (Xtensa-aware GDB bundled with ESP-IDF)
idf.py gdb
```

**Useful GDB commands in this project:**

```
bt                        # backtrace — where did the crash happen
info tasks                # list all FreeRTOS tasks and their states
thread apply all bt       # backtrace for every task
p render_stats            # inspect a global struct by name
watch frame_buf.write_idx # break when the frame buffer write index changes
```

**When to use:** panic (abort/assert), hard fault, silent hang where `idf.py monitor`
shows no output.

---

## 2. idf.py monitor (serial — firmware)

The ESP-IDF serial monitor decodes Guru Meditation errors and translates raw `PC: 0x...`
addresses to file:line using the ELF symbol table.

```bash
idf.py -p /dev/ttyUSB0 monitor
```

Panic output is decoded automatically. Example:

```
Guru Meditation Error: Core 0 panic'ed (Task watchdog got triggered)
Backtrace:
0x400d1f3c:0x3ffb1e80 render.c:112
0x400d20a4:0x3ffb1eb0 render.c:88
```

**When to use:** first tool to open after any unexpected reset or assert; always running
during hardware test sessions.

---

## 3. ESP-IDF Heap Tracing

Built-in heap tracing records every `malloc`/`free` call with a timestamp and call-stack.
Two modes are available:

| Mode | Purpose |
|---|---|
| `HEAP_TRACE_LEAKS` | Reports all allocations not freed at `heap_trace_stop()` |
| `HEAP_TRACE_ALL` | Records every call — used with the host-side `esp_heaptrace.py` visualiser |

**Enable in sdkconfig:**

```
CONFIG_HEAP_TRACING=y
CONFIG_HEAP_TRACING_STACK_DEPTH=5
```

**Instrument code:**

```c
#include "esp_heap_trace.h"

heap_trace_record_t trace_buf[64];
heap_trace_init_standalone(trace_buf, 64);

heap_trace_start(HEAP_TRACE_LEAKS);
// ... suspected code ...
heap_trace_stop();
heap_trace_dump();
```

**When to use:** `free heap` in `DIAG` output drops over time (leak suspected); any
allocation-heavy code path such as JPEG decode buffers.

---

## 4. ESP-IDF Core Dump

When `CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=y` is set, a panic writes a full core dump to
a dedicated flash partition. The dump can be retrieved and analysed after the fact without
a JTAG probe.

```bash
# Retrieve and decode
idf.py coredump-info -p /dev/ttyUSB0

# Full GDB post-mortem session
idf.py coredump-debug -p /dev/ttyUSB0
```

**When to use:** crash observed in the field or on a bench without a JTAG probe attached;
intermittent panic that is hard to catch live.

---

## 5. Valgrind — Memcheck (host simulator)

The `sim/` directory compiles the UI layer natively against SDL2 on Linux. Valgrind can
be run directly against the simulator binary.

**Build with debug symbols (already enabled in `sim/Makefile` via `-g`):**

```bash
cd sim && make
```

**Run under Valgrind (offscreen, no display required):**

```bash
SDL_VIDEODRIVER=offscreen SIM_SHOT=/tmp/sim_shot.bmp \
  valgrind --tool=memcheck \
           --leak-check=full \
           --show-leak-kinds=definite,indirect \
           --track-origins=yes \
           --error-exitcode=1 \
           ./sim
```

**Key flags:**

| Flag | Purpose |
|---|---|
| `--leak-check=full` | Report every leaked allocation with call stack |
| `--track-origins=yes` | Show where uninitialised values were created |
| `--error-exitcode=1` | Non-zero exit on any error — useful in CI |

**When to use:** any change to UI widget allocation in `scout_ui.c`; suspicion of
unfreed LVGL objects after scene transitions.

---

## 6. AddressSanitizer + UBSan (host simulator)

ASan detects out-of-bounds reads/writes and use-after-free at runtime with ~2× overhead.
UBSan catches undefined behaviour (integer overflow, misaligned access, null dereference).

**Build with sanitisers:**

```bash
cd sim
make clean
CFLAGS="-O1 -g -fsanitize=address,undefined" make
```

Or add directly to the `sim/Makefile`:

```makefile
CFLAGS += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
```

**Run (offscreen, full intro sequence):**

```bash
ASAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  SDL_VIDEODRIVER=offscreen SIM_SHOT=/tmp/sim_asan.bmp SIM_SHOT_MS=4000 ./sim
```

`SIM_SHOT_MS=4000` advances 4 000 ms of LVGL simulated time, covering the full
intro sequence before exiting cleanly.

ASan prints a detailed report on the first violation and aborts:

```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x...
READ of size 2 at 0x... thread T0
    #0 0x... in jpeg_decode_rgb565 jpeg.c:87
    #1 0x... in render_run render.c:112
```

**When to use:** after any change to frame buffer handling, JPEG decode output buffers,
or LVGL draw buffer sizing.

---

## 7. UART Diagnostic Console

Scout-Screen exposes a live diagnostic CLI on UART0 (115 200 baud). See
[`uart/uart_interface.md`](../uart/uart_interface.md) for full command reference.

**Quick reference:**

```
STATUS   uptime, free heap, free PSRAM, WiFi clients, scene
STREAM   live decode/blit/fps stats, refreshes every 200 ms (q to exit)
DIAG     FreeRTOS task count, heap watermark (lowest free heap since boot)
CAMDIAG  CAM-side heap, uptime, WiFi RSSI, BME280 temperature/humidity/pressure
```

**When to use:** performance regression (compare `STREAM` before/after a change);
memory leak hunt (watch `DIAG` min heap shrink over time); connectivity diagnosis
(check RSSI via `CAMDIAG`).

---

## 8. Automated UART Monitor Tests

`tests/test_uart_monitor.py` sends each monitor command over serial and asserts the
expected fields appear in the response. Covers: `HELP`, `STATUS`, `DIAG`, `STREAM`,
and unknown-command error handling.

```bash
pip install pyserial
python tests/test_uart_monitor.py --port /dev/ttyUSB0
```

Expected output on a passing run:

```
[HELP]
  PASS  HELP lists STATUS
  PASS  HELP lists STREAM
  PASS  HELP lists DIAG
  PASS  HELP lists HELP
[STATUS]
  PASS  STATUS header
  PASS  STATUS uptime field
  ...
13/13 checks passed
```

**When to use:** after any change to `monitor.c` or the commands it dispatches; as a
smoke test after flashing a new firmware build.
