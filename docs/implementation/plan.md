# Implementationsplan — Scout RC-bil

Baserad på analys av nuvarande kodbas mot projektkraven.
Senast uppdaterad: 2026-05-19

---

## Nuläge

| Komponent | Status |
|-----------|--------|
| WiFi AP/STA | Fungerar |
| TCP-videoström | Fungerar |
| JPEG-decode + LVGL-canvas | Fungerar |
| D-pad UI (4 knappar) | Fungerar |
| Motorstyrning riktning (IN1–IN4) | Fungerar |
| FreeRTOS task-arkitektur | Saknas — 1 task på bil-sidan, krav: minst 4 |
| FreeRTOS-kö motor commands | Saknas |
| PWM hastighetsstyrning (ENA/ENB) | Saknas — kör alltid 100% |
| Timeout-baserat nödstopp | Saknas — stannar bara vid TCP-fel |
| Heartbeat skarm till bil | Saknas |
| Watchdog (TWDT) | Saknas |
| UART-diagnostikinterface | Saknas helt |
| Telemetri bil till skarm | Saknas |
| Anslutningsstatus pa skarm | Saknas |
| Socket-timeout pa send_all | Buggrisk |
| Enhetstester | Saknas |

---

## Fas 1 — Task-arkitektur och ko (Bil-sidan)

**Mal:** Uppfyll kravet pa minst 4 separata FreeRTOS-tasks med tydliga ansvarsomraden.

### Vad som andras

Nuvarande `stream_task` gor allt: TCP-anslutning, kamerafangst, motorstyrning. Den delas upp:

```
motor_task     (prio 6) — enda task som anropar motor_apply(), vantar pa ko
network_task   (prio 5) — hanterar TCP-socket, kamerastream, tar emot kommandon
telemetry_task (prio 3) — samlar och skickar telemetridata
monitor_task   (prio 1) — loggar systemstatus periodiskt via ESP_LOGI
```

### FreeRTOS-ko

```c
// motor_command_queue: network_task skriver, motor_task laser
QueueHandle_t motor_queue = xQueueCreate(4, sizeof(uint8_t));

// network_task:
xQueueSend(motor_queue, &cmd, 0);  // aldrig blockera i natverkstask

// motor_task:
uint8_t cmd;
if (xQueueReceive(motor_queue, &cmd, pdMS_TO_TICKS(500)) == pdTRUE)
    motor_apply(cmd);
else
    motor_apply(CMD_STOP);  // nodstropp om kon ar tom > 500 ms
```

### Filer som berors

- `firmware/cam/main/stream.c` — delas upp, innehall ferdelas till nya filer
- `firmware/cam/main/motor_task.c` (ny) — motor_task() flyttas hit
- `firmware/cam/main/network_task.c` (ny) — TCP + kamera
- `firmware/cam/main/main.c` — starta alla 4 tasks

---

## Fas 2 — PWM hastighetsstyrning

**Mal:** Uppfyll kravet "timer- eller avbrottsdriven PWM-generering (ej busy-wait)".

### Vad som andras

`motor.c` styr idag bara riktningspinnarna IN1–IN4. ENA och ENB (enable/PWM-pinnar pa L298N) ar aldrig konfigurerade, vilket innebar att motorerna kor pa full effekt oavsett kommando.

```c
#define PIN_ENA  GPIO_NUM_2   // kontrollera mot er koppling
#define PIN_ENB  GPIO_NUM_4

// I motor_init():
ledc_timer_config_t timer = {
    .speed_mode      = LEDC_LOW_SPEED_MODE,
    .timer_num       = LEDC_TIMER_1,
    .duty_resolution = LEDC_TIMER_8_BIT,  // 0-255
    .freq_hz         = 25000,             // 25 kHz
    .clk_cfg         = LEDC_AUTO_CLK,
};
ledc_timer_config(&timer);
```

Utoka protokollet med ett hastighetsfalt:

```c
// rc_protocol.h
typedef struct {
    uint8_t cmd;    // CMD_FORWARD etc.
    uint8_t speed;  // 0-100 procent
} rc_cmd_t;
```

Skarmsidan skickar `rc_cmd_t` (2 bytes) istallet for 1 byte. Bil-sidan laser 2 bytes och satter PWM-duty cycle pa ENA/ENB.

### Filer som berors

- `firmware/cam/main/motor.c` och `motor.h` — lagg till LEDC + ENA/ENB
- `components/rc_protocol/rc_protocol.h` — lagg till rc_cmd_t
- `firmware/cam/main/network_task.c` — las 2 bytes istallet for 1
- `firmware/screen/main/video.c` — skicka 2 bytes
- `firmware/screen/main/ui.c` — exponera hastighet (slider eller joystick-avstand)

---

## Fas 3 — Nodstopp och heartbeat

**Mal:** Motorer stannar inom 500 ms om WiFi-anslutning bryts.

### Heartbeat

`handler_task` i `video.c` skickar redan ett kommando var 50 ms. Det racker som heartbeat — men det maste alltid skickas, aven om kommandot ar `CMD_STOP`. Utan heartbeat kan motor_task inte skilja pa "ingen input" och "WiFi borta".

### Timeout i motor_task

Kon anvands som timeout-mekanism (se Fas 1). Om `xQueueReceive` inte far nagot paket inom 500 ms triggas nododstopp automatiskt. Beror pa Fas 1.

### Socket-timeout pa bil-sidan

Nuvarande `send_all()` kan blockera oandligt om TCP-bufferten ar full:

```c
// Direkt efter connect():
struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
```

### Filer som berors

- `firmware/cam/main/network_task.c` — socket-timeout
- `firmware/cam/main/motor_task.c` — ko-timeout = nodstopp

---

## Fas 4 — Watchdog (TWDT)

**Mal:** Systemet aterhämtar sig om en kritisk task hanger.

```c
#include "esp_task_wdt.h"

// I varje kritisk task, en gang vid start:
esp_task_wdt_add(NULL);

// I varje loop-iteration:
esp_task_wdt_reset();
```

Konfigurera timeout i `sdkconfig.defaults`:

```
CONFIG_ESP_TASK_WDT=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=5
```

Tasks som ska overvakas: `motor_task`, `network_task`.

### Filer som berors

- `firmware/cam/main/motor_task.c`
- `firmware/cam/main/network_task.c`
- `firmware/cam/sdkconfig.defaults`

---

## Fas 5 — UART-diagnostikinterface

**Mal:** Minst 5 fungerande kommandon via serieporten pa bil-sidan.

### Kommandon

| Kommando | Svar |
|----------|------|
| `STATUS` | WiFi-status, IP, uptime, nodstopp-flagga |
| `MOTOR` | Aktuella PWM-varden och riktning per motor |
| `SENSOR` | Sensoravlasningar (batteri-ADC) |
| `CONFIG <param> <value>` | Andra max-hastighet, telemetriintervall |
| `DIAG` | Task-statistik, stackanvandning |
| `HELP` | Lista alla kommandon |

### Ny fil: uart_diag.c

```c
// Kors i egen task (prio 1 — lagst):
static void uart_task(void *arg)
{
    char line[64];
    while (1) {
        if (uart_readline(line, sizeof(line)) > 0)
            handle_command(line);
    }
}
```

### Filer som berors

- `firmware/cam/main/uart_diag.c` (ny)
- `firmware/cam/main/uart_diag.h` (ny)
- `firmware/cam/main/main.c` — starta uart_task
- `firmware/cam/main/CMakeLists.txt`

---

## Fas 6 — Telemetri och skarm-UI

**Mal:** Skarmen visar live-telemetri; anslutningsstatus och latens ar synligt.

### Telemetripaket bil till skarm

```c
// rc_protocol.h
#define TELEMETRY_MAGIC  0xBC

typedef struct __attribute__((packed)) {
    uint8_t  magic;
    uint8_t  direction;   // senaste CMD_*
    uint8_t  speed_pct;   // 0-100
    uint8_t  battery_pct; // 0-100 fran ADC
    uint16_t latency_ms;
} telemetry_packet_t;
```

Bilen skickar telemetripaketet var 200 ms via samma TCP-socket, efter varje frame eller i telemetry_task.

### UI-andringar pa skarmsidan

- Statusrad: WiFi-ikon + latens i ms
- Batteriindikator
- Riktnings- och hastighetsvisning
- "DISCONNECTED"-text om ingen frame mottagits pa mer an 1 sekund
- Nodstopp-knapp (rod, framtradande)

### Filer som berors

- `components/rc_protocol/rc_protocol.h` — telemetry_packet_t
- `firmware/cam/main/telemetry_task.c` (ny)
- `firmware/screen/main/video.c` — parsa telemetripaket
- `firmware/screen/main/ui.c` — telemetri-widgets + nodstopp-knapp

---

## Fas 7 — Enhetstester

**Mal:** Minst 5 enhetstester + 1 Python-testskript for UART.

### Testfiler

```
firmware/cam/test/
  test_rc_protocol.c      — CMD_*-masker, kombinationer, paketstruktur
  test_motor_logic.c      — motor_apply() sattar ratt GPIO-niver (mock)
  test_frame_protocol.c   — send/recv-protokollet med delade paket
  test_telemetry.c        — serialisering av telemetripaket
  test_emergency_stop.c   — verifierar CMD_STOP vid ko-timeout

test/
  test_uart_diag.py       — oppnar serieporten, kor STATUS/MOTOR/HELP, verifierar svar
```

---

## Sammanfattad ordning

```
Fas 1 — Task-arkitektur + ko           Allt annat beror pa detta
Fas 2 — PWM hastighetsstyrning         Kan goras parallellt med Fas 1
Fas 3 — Nodstopp + heartbeat           Beror pa Fas 1 (motor_task + ko)
Fas 4 — Watchdog                       Gor nar tasks ar stabila
Fas 5 — UART-diagnostik                Fristaende, kan goras nar som helst
Fas 6 — Telemetri + skarm-UI           Beror pa utokat protokoll fran Fas 2
Fas 7 — Enhetstester                   Lopande, klart sist
```

---

## Nya filer

| Fil | Innehall |
|-----|----------|
| `firmware/cam/main/network_task.c/.h` | TCP-anslutning, kamerastream, kommandomottagning |
| `firmware/cam/main/motor_task.c/.h` | motor_apply(), ko-konsument, nodstopp-logik |
| `firmware/cam/main/telemetry_task.c/.h` | Samla + skicka telemetripaket |
| `firmware/cam/main/uart_diag.c/.h` | UART-kommandon |
| `firmware/cam/test/test_rc_protocol.c` | Enhetstest protokoll |
| `firmware/cam/test/test_motor_logic.c` | Enhetstest motorlogik |
| `firmware/cam/test/test_frame_protocol.c` | Enhetstest frame-protokoll |
| `firmware/cam/test/test_telemetry.c` | Enhetstest telemetri |
| `firmware/cam/test/test_emergency_stop.c` | Enhetstest nodstopp |
| `test/test_uart_diag.py` | Python UART-testskript |

## Andrade filer

| Fil | Vad andras |
|-----|------------|
| `firmware/cam/main/stream.c` | Innehall ferdelas till network_task.c |
| `firmware/cam/main/motor.c/.h` | Lagg till LEDC/PWM for ENA/ENB |
| `firmware/cam/main/main.c` | Starta alla 4 tasks |
| `firmware/cam/main/CMakeLists.txt` | Nya kallfiler |
| `firmware/cam/sdkconfig.defaults` | TWDT-konfiguration |
| `firmware/screen/main/video.c` | Parsa telemetripaket, skicka rc_cmd_t |
| `firmware/screen/main/ui.c` | Telemetri-widgets, nodstopp-knapp |
| `components/rc_protocol/rc_protocol.h` | rc_cmd_t, telemetry_packet_t |
