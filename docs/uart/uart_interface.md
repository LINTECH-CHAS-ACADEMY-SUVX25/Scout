# UART Diagnostic Interface

Scout Screen exposes a diagnostic CLI on **UART0** (USB serial port).
Connect with any serial terminal at **115 200 baud, 8N1**.

```
idf.py -p /dev/ttyUSB0 monitor
# or with e.g. minicom / PuTTY at 115200 8N1
```

On startup the system prints:

```
Scout monitor — type HELP
```

Type a command and press Enter. All commands are case-sensitive.

---

## Commands

### STATUS

Prints a snapshot of the system's current operating state.

**Syntax:** `STATUS`

**Response:**

```
--- STATUS ---
uptime       42s
free heap    189432B
free PSRAM   3801088B
WiFi clients 1
cam stream   connected
scene        streaming
```

| Field | Description |
|---|---|
| `uptime` | Seconds since boot |
| `free heap` | Free internal heap memory |
| `free PSRAM` | Free external PSRAM |
| `WiFi clients` | Number of connected STA clients (0 = car not connected) |
| `cam stream` | `connected` if video frames are being received, otherwise `disconnected` |
| `scene` | Current scene: `waiting`, `streaming`, or `disconnected` |

---

### STREAM

Displays live performance statistics for the video and rendering pipeline.
Updates every 200 ms. Exit with the **q** key.

**Syntax:** `STREAM`

**Response (example):**

```
=== STREAM ===
Receive             last       avg
  frames      152
  frame size  18432B    17980B
  transfer    28ms        30ms
  frame gap   33ms        34ms
  max fps     30.3fps    29.4fps
  loop         1ms         1ms
Render              last       avg
  lvgl        12ms        11ms
  decode       8ms         9ms
  blit         4ms         4ms
  fps         28.5fps    27.9fps
  loop        25ms        25ms
```

| Section | Field | Description |
|---|---|---|
| Receive | `frames` | Total frames received since boot |
| Receive | `frame size` | Size of the JPEG frame in bytes |
| Receive | `transfer` | Time from first to last fragment per frame |
| Receive | `frame gap` | Time between two complete frames |
| Receive | `max fps` | Theoretical maximum framerate (1000 / frame gap) |
| Receive | `loop` | Iteration time of the stream task's main loop |
| Render | `lvgl` | Time for LVGL to redraw dirty areas |
| Render | `decode` | Hardware JPEG decode time |
| Render | `blit` | DMA transfer time to the LCD panel |
| Render | `fps` | Actual display framerate |
| Render | `loop` | Iteration time of the render task's main loop |

`last` = most recent measurement, `avg` = rolling average.

---

### DIAG

Prints heap and FreeRTOS task diagnostics.

**Syntax:** `DIAG`

**Response:**

```
--- DIAG ---
tasks       6
min heap    142680B
free int    189432B
free PSRAM  3801088B
```

| Field | Description |
|---|---|
| `tasks` | Number of active FreeRTOS tasks |
| `min heap` | Lowest free heap recorded since boot (watermark) |
| `free int` | Current free internal heap memory |
| `free PSRAM` | Current free PSRAM |

---

### CAMDIAG

Displays diagnostic data from the car (ESP32-CAM): heap, WiFi signal strength,
and BME280 sensor readings. Data is sent by the camera embedded in the video stream.

**Syntax:** `CAMDIAG`

**Response:**

```
--- CAM DIAG ---
heap        38420B
uptime      45s
rssi        -52dBm
temp        23.4°C
humidity    48.2%
pressure    1013.2hPa
```

| Field | Description |
|---|---|
| `heap` | Free heap memory on the ESP32-CAM |
| `uptime` | Seconds since CAM boot |
| `rssi` | WiFi signal strength (dBm) — more negative = weaker signal |
| `temp` | Ambient temperature from BME280 |
| `humidity` | Relative humidity from BME280 |
| `pressure` | Air pressure from BME280 |

Requires the camera to be connected and streaming. If no data is available, zero values are shown.

---

### HELP

Lists available commands.

**Syntax:** `HELP`

**Response:**

```
commands:
  STATUS   uptime, heap, WiFi clients, stream connection, scene
  STREAM   live stream stats (q to exit)
  DIAG     heap watermarks, task count
  CAMDIAG  cam heap, uptime, RSSI, sensor data
  HELP     this list
```

---

## Error message

An unknown command produces:

```
unknown command 'FOO' — try HELP
```
