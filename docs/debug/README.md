# Debugging — Overview

Documentation of debugging work performed in the Scout project.

## Structure

| File | Contents |
|---|---|
| [software_tools.md](software_tools.md) | Tool reference: GDB, Valgrind, ASan/UBSan, ESP-IDF heap tracing, idf.py monitor |
| [findings.md](findings.md) | Documented debugging results per issue |

## System under test

| Device | Chip | Role |
|---|---|---|
| **Scout-CAM** | ESP32-CAM (Xtensa LX6, 240 MHz) | Vehicle — camera, motor control, BME280 sensor, WiFi STA |
| **Scout-Screen** | ESP32-S3 (Xtensa LX7, dual-core 240 MHz) | Dashboard — LCD display, LVGL rendering, UDP receive, UART console |

The project has two testable targets:

- **Firmware** — cross-compiled for ESP32 via ESP-IDF; debugged with OpenOCD/GDB and
  runtime ESP-IDF instrumentation (heap tracing, core dumps, task watchdog).
- **Host simulator** (`sim/`) — UI code compiled natively with GCC/Clang against SDL2;
  fully compatible with Valgrind, AddressSanitizer, and GDB on Linux.
