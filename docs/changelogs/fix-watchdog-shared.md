# fix/watchdog-shared

Watchdogen övervakade tidigare bara cam-sidan. Nu ligger den i `shared_components`
och bevakar båda korten, och en hängd task startar om systemet istället för att bara
logga.

## Flyttat

- `scout_cam/components/watchdog/` → `shared_components/watchdog/` så att både
  scout_cam och scout_screen använder samma komponent

## Ändrat

- `watchdog.c/h` — ny `watchdog_init()` som armar TWDT med explicit timeout
  (`WATCHDOG_TIMEOUT_MS 5000`) och `trigger_panic = true`, så en hängd task rebootar
  och systemet återhämtar sig. Bevakar även varje kärnas idle-task så en skenande
  högprio-task som svälter ut en kärna också fångas. Kör `esp_task_wdt_init` och
  faller tillbaka på `esp_task_wdt_reconfigure` om IDF redan auto-startat TWDT.
- `scout_cam/main/main.c`, `scout_screen/main/main.c` — `watchdog_init()` anropas
  först i `app_main`, före att någon task registrerar sig
- `scout_screen/main/render.c` — `render`-tasken registrerar sig nu med watchdogen
  och kallar `watchdog_reset()` varje loop-iteration. Det är den enda tysta hängningen
  på screen-sidan (fryst skärm)
- `scout_screen/main/CMakeLists.txt` — `watchdog` tillagd i REQUIRES

## Medvetet utelämnat

- `stream` (udp_rx) och `monitor` (UART) på screen registreras inte — de blockerar
  avsiktligt på I/O och skulle ge falska reboots när kameran är frånkopplad eller
  ingen skriver i konsolen
