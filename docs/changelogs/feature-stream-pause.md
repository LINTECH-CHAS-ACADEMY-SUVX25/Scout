# feature/stream-pause

Löste WiFi deauth-spam och ett deadlock vid screen-reboot. Lade till stream-paus och
omstrukturerade cam-sidan så att stream-tasken följer samma mönster som render-tasken
på screen-sidan.

## Rotsak: deauth-spam

Cam fortsatte streama UDP-paket under AP-nedgång. Varje paket triggade ett deauth-svar
från AP:n → cam kickades direkt vid nästa reconnect-försök → loop med 30+ deauths i rad.
Stream-pausen bryter loopen: efter 150 tysta frames (ca 5 s) slutar cam sända helt.
Verifierat på enhet — deauth-skuren är borta.

## Rotsak: deadlock vid screen-reboot

Cam pausad → väntade på kommando för att återuppta. Screen rebootad → visste inte cam-IP
förrän en frame anlänt. Ingen av dem kunde ta första steget. Lösning: cam återupptar
stream vid WiFi-reconnect utan att invänta ett kommando.

## Nytt

- `components/wifi_sta` — `wifi_sta_is_connected()` exponerar anslutningsstatus för
  stream-tasken; `s_connected` sätts i event-handlens disconnect/got-ip-grenar

## Ändrat

- `adapters/cam_state` — utökad från ren datahållare till state manager (analogt med
  `screen_state` på screen-sidan): äger `s_silent_frames` och `s_reconnect_pending` som
  privata statics; exponerar `cam_state_try_resume(sock)` och `cam_state_process_cmds(sock)`
  som hanterar pause/resume-logiken och screen liveness-tracking

- `main/stream.c` — reducerad till rena adaptranrop; all inline logik (reconnect-detektion,
  command drain, liveness-räknare) flyttad till `cam_state`; `wifi_sta.h` inkluderas inte
  längre direkt i task-filen

- `sdkconfig.defaults` — `CONFIG_LOG_DEFAULT_LEVEL_INFO=y` så att INFO-loggar kompileras
  in och syns i UART-monitorn
