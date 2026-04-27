# Scout Simulator – LVGL 1024×600

PC-simulator för Scout RC-bilen. Kör samma C-kod och LVGL-UI som den
riktiga enheten, men med SDL2 istället för TFT-drivrutin och simulerad
sensordata istället för I²C/SPI.

Matchar **Waveshare 7" LCD (1024×600)** med **ESP32-CAM (640×480)**.

## Layout

```
┌──────── 640 px ────────┬────── 384 px ──────┐
│                        │  TELEMETRI  100% 42s│
│   CAMERA FEED          │  ┌──────┬──────┐   │
│   640 × 480            │  │ MILJÖ│ GAS  │   │
│                        │  ├──────┼──────┤   │
│   [SCOUT] [▼ DATA] T°C│  │TERMSK│ IMU  │   │
│   ─ HUD overlay ─      │  ├──────┼──────┤   │
│                        │  │PARTIK│RANGE │   │
│                        │  └──────┴──────┘   │
│                        │  [═══ STATUS: OK ══]│
├────────────────────────│                     │
│ ┌──┐┌──┐┌──┐┌──┐┌──┐  │  [LAMPOR]  ╭─────╮ │
│ │CO││CO2│PM25│DST│LUX│  │  [REC  ]  │ JOY │ │
│ └──┘└──┘└──┘└──┘└──┘  │  [BILD ]  │STICK│ │
│        STYR  GAS       │           ╰─────╯ │
└────────────────────────┴─────────────────────┘
```

## Vad simuleras?

| Sensor         | Modell            | Data                              |
|----------------|-------------------|-----------------------------------|
| Miljö          | BME280            | Temperatur, luftfuktighet, tryck  |
| Gas            | MQ-2 / MiCS-6814 | CO, CO₂, NH₃, TVOC               |
| Partiklar      | PMS5003           | PM2.5, PM10                       |
| IMU            | MPU-6050          | Roll, pitch, yaw + kompass        |
| Avstånd        | VL53L0X           | cm + hinderdetektion              |
| Termisk kamera | AMG8833           | 8×8 värmekarta                    |
| Ljus           | BH1750            | Lux                               |

Simulatorn kör slumpmässiga events: **gasläcka**, **hinder**, **värmekälla**.
Alert-nivån uppdateras automatiskt: OK → VARNING → FARA.

## Förutsättningar

### Ubuntu / Debian
```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev git
```

### macOS
```bash
brew install cmake sdl2
```

### Windows (WSL2 - Rekommenderat)
```bash
# Installera WSL2 med Ubuntu från Microsoft Store
# Sedan i WSL2-terminalen:
sudo apt update
sudo apt install build-essential cmake libsdl2-dev git

# För att köra GUI (SDL-fönster) från WSL2:
# Installera VcXsrv eller X410 på Windows
export DISPLAY=:0
```

### Windows (MSYS2 - Alternativ)
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 git
```

## Bygg & kör

```bash
git clone https://github.com/<team>/scout-sim.git
cd scout-sim

# Hämta LVGL v8 submoduler
git submodule update --init --recursive

# Bygg
mkdir build && cd build
cmake ..
make -j$(nproc)

# Kör – öppnar ett 1024×600 SDL-fönster
./scout_sim
```

## Interaktion

- **Joystick**: Klicka och dra i joystick-ringen (nedre höger) för att styra.
  Styr- och gas-procent visas i bottom bar.
- **▼ SENSORDATA**: Klicka för att toggla detaljerad sensoröverlagring på kameran.
- **LAMPOR / SPELA IN / BILD**: Klickbara actionknappar i höger panel.

## Projektstruktur

```
scout-sim/
├── CMakeLists.txt
├── lv_conf.h                # LVGL config (1024×600)
├── lv_drv_conf.h            # LVGL drivers config (SDL)
├── include/
│   ├── sensor_hal.h         # Sensor HAL (sim + hw)
│   └── ui_dashboard.h       # Dashboard API
├── src/
│   ├── main.c               # SDL entry point + joystick input
│   ├── sensor_sim.c         # Simulerad sensordata
│   └── ui_dashboard.c       # LVGL UI – alla paneler, dropdown, joystick
├── lvgl/                    # (submodule) LVGL v8.3
└── lv_drivers/              # (submodule) SDL driver v8.3
```

## Arkitektur: Sim → Hårdvara

```
┌──────────────────────────────────────┐
│         ui_dashboard.c               │  Identisk på PC och MCU
│  (LVGL layout, widgets, färger)      │
├──────────────────────────────────────┤
│           sensor_hal.h               │  Gemensamt API
├─────────────────┬────────────────────┤
│  sensor_sim.c   │  sensor_hw.c       │
│  (SDL / PC)     │  (ESP32 / STM32)   │
│  fake data      │  riktig I²C / SPI  │
├─────────────────┼────────────────────┤
│  SDL display    │  ILI9341 / ST7789  │
│  SDL mouse      │  Touchpanel / BLE  │
└─────────────────┴────────────────────┘
```

Vid portning:
1. Ersätt `sensor_sim.c` med `sensor_hw.c` (riktiga I²C-anrop)
2. Byt SDL-drivrutinen mot er TFT-drivrutin
3. Koppla kameravyn till ESP-CAM MJPEG-ström
4. Koppla joystick-input till BLE/WiFi-kontroller

## Anpassning

- **Tröskelvärden**: `scout_evaluate_alerts()` i `sensor_sim.c`
- **Nya paneler**: Lägg till i `ui_dashboard_create()` / `_update()`
- **Upplösning**: Ändra i `lv_conf.h` + `ui_dashboard.h`
- **Färgschema**: `C_*` defines i `ui_dashboard.c`

## Licens

MIT
