# fix/simplify-codebase-screen

## Tillagt
- `stream.c/.h` — TCP-server, mottagning av MJPEG-frames, JPEG-avkodning. Exponerar `stream_init()`, `stream_get_client_sock()`, `stream_try_decode()`
- `render.c/.h` — Render-task som driver LVGL-renderloop, skickar RC-kommandon och triggar canvas-uppdatering vid ny frame
- `lvgl_port.c/.h` — Isolerar all LVGL-kod: drivrutinsregistrering, tick-task, widget-träd, canvas-hantering. Enda filen som inkluderar `lvgl.h`

## Ändrat
- `display.c/.h` — Renodlad till ren hårdvaruinitiering (panel, touch, framebuffers, bakgrundsljus). Exponerar `display_get_panel()`, `display_get_touch()`, `display_get_fb()` som `void*`
- `ui.c/.h` — Renodlad till tillståndslogik utan LVGL-beroende. Hanterar kommandon, anslutningsstatus, FPS och sensorsimulering. Delegerar alla UI-uppdateringar till `lvgl_port`
- `ui.h` — Nya funktioner `ui_input_event(int dx, int dy)` och `ui_input_release()` för abstrakt joystick-inmatning
- `main.c` — Förenklad startsekvens: `wifi_ap_start → display_init → ui_init → stream_init → render_init`
- `CMakeLists.txt` — Uppdaterad med `stream.c`, `render.c`, `lvgl_port.c`
- `render.c` — Renderloggar endast när en frame tar >5ms, eliminerar loggspam vid idle

## Borttaget
- `video.c/.h` — Ersatt av `stream.c` och `render.c`

## Arkitektur
LVGL är nu isolerat till `lvgl_port.c`. Övriga filer (`render.c`, `stream.c`, `ui.c`, `display.c`) inkluderar inte `lvgl.h` och kan modifieras eller bytas ut utan kännedom om UI-ramverket.

------------------------------------------ EJ AI-PROMPTADE KOMMENTARER HÄR ------------------------------------------

Jag har försökt att modulera screen arkitekturen så logiskt som jag kunde göra på en kväll

Ändrat namn på vissa filer då dom var ologiska

Finns grejer kvar att göra, kommentarer längst uppe i .h filerna å över funktioner är en grej

LVGL ska optimeras eller UT!!!!!!!! får en average på 400ms render tid, helt orimligt faktiskt

Simulerad sensor data finns i ui.c - bör tas bort då sensorn ska sitta på SCOUT_CAM
Om vi inte fixar att montera bme280'n(temperatur sensoren) till cam så får vi eventuellt simulera datan ändå
Fast den datan bör simuleras i cam och inte i screen, sen kan vi skicka mock datan på samma sätt som
om vi hade en riktig sensor. Implementera Gustav's HAL lr nåt jag vet inte >:)

Funderar på att plocka ut lvgl_port ifrån render.c och istället köra dem funktionerna via ui.c om det går
Helst vill jag att bara ui.c vet att lvgl_port.c finns, på så sätt blir det lättare att ta bort skiten om
det finns möjlighet för det. INGEN fil förutom lvgl_port ska inkludera lvgl ------ ABSOLUT FÖRBUD!!! >:(

Upprepar lite från TODO.md ifall ni vill jobba på något av de:
Istället för att bara skicka info från screen->cam på joy_event (kan vara ui_input_event/ui_input_release är osäker)
så bör vi pinga med jämna mellanrum så vi kan ha ett nödstopp på cam ifall vi inte får en ping på x antal millisekunder

wifi.c bör överses om det finns sätt att minska dess minnesupptagning i SRAM (Interna RAM-minnet)
behöver vi starta ett wifi-AP eller kan vi överföra data på något smidigare sätt? 
(obs kevin pratar inte om udp här). Vi har 500KB SRAM och wifi.c tar upp 300KB av det.

//ALEX

------------------------------------------ KLÅDS FLOWCHART FÖR SCOUT_SCREEN ------------------------------------------
app_main()
├── wifi_ap_start()        — starts WiFi access point
├── display_init()         — initialises LCD panel, touch controller, framebuffers
├── ui_init()              — builds LVGL widget tree (calls lvgl_port_init internally)
├── stream_init()          — creates mutexes, starts tcp_server_task
├── render_init()          — creates video canvas, starts render_task
└── vTaskDelete(NULL)      — app_main exits


════════════════════ FreeRTOS tasks (run forever) ════════════════════

tick_task  [prio 5]
└── every 10ms: lv_tick_inc(10)

render_task  [prio 4]
└── every 10ms:
    ├── ui_tick()              — update sensor sim, push state changes to widgets
    ├── run_render_frame()     — drive LVGL render pass (flush to display if dirty)
    ├── send_rc_command()      — send joystick command byte over TCP to car
    └── try_decode_frame()     — if new JPEG arrived: decode into canvas buffer, mark dirty

tcp_server_task  [prio 3]
└── accept() → loop:
    ├── recv_frame_header()    — read magic byte + length
    ├── try_store_frame()      — receive JPEG bytes into buffer  (if decoder free)
    └── drain_frame()          — discard bytes silently          (if decoder busy)


════════════════════ Event-driven (LVGL touch callbacks) ════════════════════

joy_event()  [fires on touch press/release]
├── PRESSING:   calculate dx/dy → ui_input_event(dx, dy) → s_cmd updated
└── RELEASED:   ui_input_release() → s_cmd = STOP