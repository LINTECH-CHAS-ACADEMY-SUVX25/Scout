# fix/watchdog-cam-recovery

Två separata kraschar efter smutsig reset på cam-sidan åtgärdade: watchdogen timeout:ade
under WiFi-scan, och kameran abortade vid init om I2C-bussen var låst sedan förra körningen.

## Ändrat

- `shared_components/watchdog/watchdog.h` + `watchdog.c` — `watchdog_init()` tar nu en
  valfri `watchdog_config_t *` (NULL ger tidigare standardbeteende). Fälten `timeout_ms`
  och `idle_core_mask` kan sättas per projekt.

- `scout_cam/main/main.c` — skickar `wtd_cfg` med `idle_core_mask = 0` så att cam-sidans
  watchdog inte bevakar idle-taskarna. WiFi-drivrutinen (prio 23) svälter ut idle-tasken
  under 14-kanalsskanningar, vilket gav falska `TG1WDT_SYS_RESET`. Screen-sidan är opåverkad.

- `scout_screen/main/main.c` — uppdaterat till `watchdog_init(NULL)` för den nya signaturen;
  beteendet är oförändrat.

- `scout_cam/components/camera/camera.c` — `ESP_ERROR_CHECK` på `esp_camera_init` ersatt med
  en explicit felkontroll + `esp_restart()`. I2C-bussen kan fastna efter smutsig reset och
  orsaka att kameran misslyckas med att initieras; en kontrollerad reboot återställer bussen
  istället för att aborta mitt i uppstarten.
