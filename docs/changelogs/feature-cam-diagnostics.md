# feature/cam-diagnostics

Scout_cam skickar ett UDP-diagnostikpaket till scout_screen var 2:a sekund. Scout_screen
exponerar datan via CAMDIAG-kommandot i UART-monitorn. Verifierat på hårdvara: heap, uptime
och RSSI visar riktiga värden. Sensorfälten (temp/humidity/pressure) är nollstubbade tills
BME280 är inkopplad (#54).

## Nytt

- `shared_components/rc_protocol/rc_protocol.h` — lade till `DIAG_PORT 3336` och packed
  struct `cam_diag_pkt_t` (temp_cdeg, humidity_pct, pressure_pa, free_heap, rssi_dbm, uptime_s)

- `scout_cam/components/wifi_sta` — lade till `wifi_sta_get_rssi()` som hämtar RSSI via
  `esp_wifi_sta_get_ap_info()`

- `scout_cam/main/adapters/cam_state` — lade till `rssi_dbm` i `cam_status_t` och
  `cam_state_update_rssi()` som anropar wifi_sta_get_rssi()

- `scout_cam/main/telemetry.c` (ny fil) — FreeRTOS-task som var 2000 ms fyller i
  `cam_diag_pkt_t` och skickar till S3_IP:DIAG_PORT via UDP

- `scout_screen/main/adapters/cam_diag.c` (ny fil) — FreeRTOS-task pinnead till core 0
  som blockerar på udp_rx(DIAG_PORT) och uppdaterar global `cam_diag_latest`

## Ändrat

- `scout_cam/main/main.c` — anropar `telemetry_init()` efter `stream_init()`

- `scout_screen/main/monitor.c` — anropar `cam_diag_init()` i `monitor_init()`

- `scout_screen/main/adapters/monitor_cmds.c` — lade till CAMDIAG-kommando som skriver ut
  heap, uptime, RSSI, temp, humidity och pressure; lade till CAMDIAG i HELP-texten
