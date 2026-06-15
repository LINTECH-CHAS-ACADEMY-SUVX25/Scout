# Scout

[![Build](https://github.com/LINTECH-CHAS-ACADEMY-SUVX25/Scout/actions/workflows/build.yml/badge.svg)](https://github.com/LINTECH-CHAS-ACADEMY-SUVX25/Scout/actions/workflows/build.yml)

Scout is a remotely operated ground vehicle built for situations where you want eyes and sensor data somewhere before a person goes in. That could be a confined space with a suspected gas leak, a building after a fire, a collapsed structure, or any emergency where getting a read on the environment first makes sense. An operator drives Scout in remotely from a 7-inch touchscreen while watching live video and monitoring the environment in real time.

The vehicle carries a camera and a BME280 sensor that continuously reads temperature, humidity, and air pressure. That data is streamed back to the dashboard alongside the video feed. The platform is built to be extended with whatever sensors the situation calls for.

## Hardware

| Component | Hardware | Role |
|---|---|---|
| Dashboard | ESP32-S3-Touch-LCD-7B | Touchscreen UI, virtual joystick, live video and telemetry display |
| Vehicle | ESP32-CAM (AI-Thinker) | Camera, motor control, sensor readout |
| Motor driver | L298N H-bridge | Controls the two DC drive motors |
| Environment sensor | BME280 | Temperature, humidity and air pressure |
| Chassis | Robot Car Kit 2WD | Two-wheel drive base |

## How it works

The dashboard starts a WiFi access point (`Scout_AP`) and the vehicle connects to it on boot. Once connected, video flows from the vehicle to the dashboard over UDP on port 3334, and joystick commands go back the other way on port 3335. Keeping video and commands on separate channels means a burst of large video frames does not delay steering input.

```
Dashboard (AP)  <-- UDP 3334 --  Vehicle (STA)   JPEG video fragments
Dashboard (AP)  --- UDP 3335 --> Vehicle (STA)   joystick commands (x, y)
```

The dashboard decodes incoming JPEG frames in hardware and blits them directly to the LCD. The virtual joystick on the touchscreen sends x/y coordinates at up to 5 Hz, which the vehicle translates into motor direction and speed via an L298N H-bridge. If no command arrives within 500 ms the motors stop automatically as a safety measure.

Environmental data from the BME280 is bundled into the video stream as a small telemetry packet and displayed on the dashboard alongside the video feed.

## UI simulator

Working on the dashboard UI does not require physical hardware. The `sim/` folder contains an LVGL simulator that runs the UI on a PC via SDL2. It compiles the exact same UI source file (`lvgl_port_ui.c`) that the firmware uses, so there is no copy to keep in sync — if it looks right in the simulator, the firmware is already updated.

```bash
cd sim
make run
```

Requires SDL2 (`sudo apt install libsdl2-dev` on Debian/Ubuntu). The first build takes a moment since it compiles all of LVGL; subsequent builds only rebuild what changed.

Once running, the joystick is controlled with the mouse. Additional keyboard shortcuts:

| Key | Action |
|---|---|
| `c` | Cycle WiFi signal strength (0-3 bars) |
| `t` | Cycle UI theme (Sonar, Desert, Night Ops) |
| `q` / `Esc` | Quit |


## Repository structure

```
scout_cam/               ESP32-CAM firmware (vehicle)
  main/
    adapters/            frag_tx, motor_cmd, cam_state
  components/            camera, l298n, wifi_sta, bme280
scout_screen/            ESP32-S3 firmware (dashboard)
  main/
    adapters/            frag_rx, frame_buf, cam_cmd, monitor_cmds, scene
  components/            display, lvgl_port, touch, uart_console, wifi_ap, ui
sim/                     LVGL desktop simulator (SDL2, no hardware needed)
shared_components/       rc_protocol, udp, jpeg, watchdog
test/                    Host-side unit tests (C++)
tests/                   Integration tests (Python)
docs/
  diagrams/              Architecture, state, program flow and sequence diagrams
  uart/                  UART diagnostic interface reference
  changelogs/            Per-branch changelogs
  scout_cam_flow.md      Vehicle architecture and task dependency graph
  scout_screen_flow.md   Dashboard architecture and task dependency graph
```

## Build and flash

Requires [ESP-IDF v6.0.1](https://github.com/espressif/esp-idf/tree/v6.0.1).

**Vehicle (ESP32-CAM):**
```bash
cd scout_cam
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

**Dashboard (ESP32-S3):**
```bash
cd scout_screen
idf.py build
idf.py -p /dev/ttyUSB1 flash monitor
```

Exit the monitor with `Ctrl+T` then `Ctrl+X`.

## Starting the system

1. Flash and start the dashboard. It will bring up the WiFi AP.
2. Flash and start the vehicle. It connects to the AP automatically.
3. Once the vehicle connects, live video appears on the screen and the joystick is active.

## UART diagnostics

The dashboard has a serial CLI on UART0 at 115200 baud that lets you inspect stream performance, heap usage, task counts and live sensor data without attaching a debugger. Connect any serial terminal at 115200 8N1 and type `HELP` to get started. Full reference in [docs/uart/uart_interface.md](docs/uart/uart_interface.md).

## Contributing

See [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) for branch naming, commit style and PR flow.
