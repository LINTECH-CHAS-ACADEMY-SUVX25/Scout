# 2026-05-26 — Motor, monitor & telemetri (main branch)

## Konfliktlösning
- Pullade senaste `main` som innehöll teamets refaktorering (`motor_task.c`, `network_task.c` ersatte `stream.c`).
- Löste merge-konflikter i `CMakeLists.txt`, `main.c` och `stream.c`.
- `stream.c` borttagen — ersatt av `network_task.c` från teamets refaktorering.

## Motorstyrning (`motor.c`)
- Rensade bort dubblerad queue-logik (hanteras nu av `motor_task.c`).
- Behöll tank-styrningslogik: stöd för kombinerade kommandon (framåt + vänster = kurva).
- Konflikthantering: motstridiga kommandon (framåt + bakåt) nollställer varandra.

## Övervakning (`monitor.c` / `monitor.h`)
- Ny task som loggar ledigt heap och antal aktiva FreeRTOS-tasks var 5:e sekund.

## Telemetri (`telemetry.c` / `telemetry.h`)
- Placeholder-task tillagd, redo att byggas ut.

## Startsekvens (`main.c`)
- `monitor_start()` och `telemetry_start()` tillagda i `app_main()`.
- Startar nu i ordning: `motor_init` → `wifi_connect` → `camera_init` → `monitor_start` → `telemetry_start` → `motor_task_start` → `network_task_start`.
