# feature/signal-lost-screen

När scout_cam slutar skicka frames täcks kameraregionen av en "SIGNAL LOST"-overlay.
Overlaysen försvinner direkt när streaming återupptas. Ingen ny global state —
render-tasken trackar `was_streaming` lokalt och kallar port-lagret vid förändring.

## Tillagt

- `scout_screen/components/lvgl_port/lvgl_port.h` — deklaration av `lvgl_port_signal_lost(bool lost)`

- `scout_screen/components/lvgl_port/lvgl_port.c` — svart overlay positionerad över
  kameraregionen (`LEFT_W, CONTENT_Y, VIDEO_W, CONTENT_H`) med "SIGNAL LOST"-text,
  skapad dold i `lvgl_port_ui_init()`; `lvgl_port_signal_lost()` visar/döljer den

## Ändrat

- `scout_screen/main/render.c` — trackar `was_streaming` (init `false`), kallar
  `lvgl_port_signal_lost(!streaming)` vid förändring; tog bort TODO-kommentaren
