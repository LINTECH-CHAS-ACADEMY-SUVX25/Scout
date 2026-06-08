# HAL-implementation + CAM-monitor

## Bakgrund

Två branches slogs ihop: `feature/hal-impl-cam` och `feature/cam-monitor`.

Problemet som löstes: tasks anropade hårdvarufunktioner direkt, vilket gjorde det omöjligt att byta ut en komponent utan att ändra i alla filer som använde den. Kevins poäng om adapter-mönstret var korrekt — en task ska aldrig behöva veta vad som sitter under.

## HAL — Hardware Abstraction Layer

### Nya klasser

- `DisplayEsp32` — implementerar `IDisplay`, delegerar till `display.h`
- `NetworkEsp32` — implementerar `INetwork`, delegerar till `udp.h`
- `CameraEsp32` och `MotorEsp32` existerade redan men var inte kopplade

### Koppling till tasks

Tasks beror nu enbart på interface-pekare. `main.cpp` är enda filen som känner till konkreta typer.

| Task | Förr | Nu |
|---|---|---|
| `motor_task` | anropade `motor_apply()` direkt | anropar `s_motor->apply()` via `IMotor*` |
| `udp_stream` | anropade `camera_capture()` direkt | anropar `s_camera->capture()` via `ICamera*` |
| `render` | anropade `display_get_fb()` + memcpy | anropar `s_display->drawBitmap()` via `IDisplay*` |

För att byta hårdvara: skapa ny klass som implementerar rätt interface, ändra en rad i `main.cpp`.

### Filbyten

Följande filer döptes om för att möjliggöra C++-klasser:

- `motor_task.c/.h` → `motor_task.cpp/.hpp`
- `udp_stream.c/.h` → `udp_stream.cpp/.hpp`
- `render.c/.h` → `render.cpp/.hpp`
- `main.c` (båda sidor) → `main.cpp`

### scout_hal CMakeLists

Mockar kompileras inte längre in i firmware-bygget. `scout_hal` exponerar bara headers — mockarna används enbart av test-bygget.

## CAM-monitor

Interaktiv UART-diagnostik på cam-sidan (UART0, 115200 baud).

### Kommandon

| Kommando | Beskrivning |
|---|---|
| `STATUS` | Uptime, heap, PSRAM, WiFi, motorstatus |
| `MOTOR` | Senaste kommando, riktning per motor |
| `SENSOR` | WiFi RSSI, kanal, frames skickade |
| `DIAG` | Task-antal, heap-vattenståndsmarkering |
| `CONFIG <p> <v>` | Konfigureringsplats (utbyggbar) |
| `HELP` | Kommandolista |

### Nya funktioner

- `motor_task_get_last_cmd()` — returnerar senast utförda motorkommando
- `udp_stream_get_stats()` — returnerar `frames_sent` och `last_frame_bytes`
