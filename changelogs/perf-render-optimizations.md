# perf/render-optimizations

### Render task wakeup via task notification
**Filer:** `render.c`, `frame_buf.c`, `frame_buf.h`

Ersatte `vTaskDelay(1)` med `ulTaskNotifyTake` i render-loopen. `frame_buf_publish` anropar
nu `xTaskNotify` direkt när en frame swappas in, så render-tasken vaknar omedelbart istället
för att polla i 100 Hz. Fallback på 20 ms timeout säkerställer att LVGL-touch och animationer
fortfarande hanteras när ingen stream är aktiv.

---

### LVGL draw buffer ökad från 20 till 120 rader
**Fil:** `lvgl_port.c`

Minskar antalet flush-pass vid tema-byte från ~30 till ~5. Vanliga dirty-area-renders
(badge-uppdateringar, link-label) påverkas inte.

---

### Guard mot redundanta `update_cmd_badges`-anrop
**Fil:** `scout_ui.c`

`LV_EVENT_PRESSING` triggas varje indev-poll (~10 ms) medan joysticken hålls inne.
Lade till en dirty-check så att `update_cmd_badges` hoppar över 15 `lv_obj_set_style_*`-anrop
när kommandovärdet inte förändrats. Guarden nollställs vid varje UI-rebuild så att
tema-byten fortfarande målar om badges korrekt.
