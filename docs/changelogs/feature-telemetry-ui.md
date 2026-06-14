# feature/telemetry-ui

Kopplar in de mottagna BME280-värdena (temp/fukt/tryck) i LVGL-UI:t på scout_screen.
Transporten var redan klar; denna branch lägger till UI-halvan.
**Verifierad på enhet**: TELEMETRI-panelen visar live-värden som matchar CAMDIAG-utskriften;
värdena uppdateras ~var 2 s; tema-byte nollställer inte panelen.

## Nytt

- `scout_screen/components/cam_diag_fmt/` (ny komponent) — delade formatters för
  wire→människa-konvertering av `cam_diag_pkt_t`-fälten. Tre funktioner:
  `cam_diag_fmt_temp` ("27.1 C"), `cam_diag_fmt_humi` ("36 %"), `cam_diag_fmt_pres`
  ("1002 hPa"). Integer-only (ingen `%f`); sign-säker för negativa temperaturer nära 0°C.
  Anropas av både fälttabellen i `scout_ui.c` och CAMDIAG-kommandot — konverteringslogiken
  definieras på ett enda ställe.

## Ändrat

- `scout_screen/components/ui/scout_ui.c` — deklarativ `tele_field_t`-tabell parar ihop
  `s_val_temp/humi/pres`-widgetarna med sina formatters. `scout_ui_update_telemetry(d)` kör
  ett generiskt update-pass: formaterar varje fält, jämför med cachad sträng (`last[]`) och
  anropar `lv_label_set_text` enbart om texten förändrats — bara berörda label-rektanglar
  invalideras. `scout_ui_set_theme()` nollställer `last[]` och anropar `update_telemetry`
  med det cachade `s_cam_diag`-paketet direkt efter att widgetarna återskapats, så värdena
  aldrig återgår till platshållare efter ett temabyte.

- `scout_screen/components/ui/scout_ui.h` — deklarerar `scout_ui_update_telemetry`.

- `scout_screen/main/adapters/screen_state.c/.h` — `screen_state_set_cam()` sätter nu
  `s_cam_dirty = true`; ny `screen_state_cam_dirty_take()` returnerar och nollställer flaggan
  (lock-free, same discipline som `screen_state_set/get_scene`).

- `scout_screen/main/render.c` — konsumerar dirty-flaggan varje tick; läser paketet och
  anropar `scout_ui_update_telemetry` enbart när ett nytt paket anlänt. Idle-ticks gör
  enbart en flaggläsning.

- `scout_screen/main/adapters/monitor_cmds.c` — `cmd_camdiag` använder nu
  `cam_diag_fmt_temp/humi/pres` från `cam_diag_fmt`; temp/fukt/tryck-raderna formateras
  konsekvent med UI-panelen (temp "27.1 C", tryck i hPa).

- `scout_screen/components/ui/CMakeLists.txt`, `scout_screen/main/CMakeLists.txt` — lade
  till `cam_diag_fmt` i REQUIRES.
