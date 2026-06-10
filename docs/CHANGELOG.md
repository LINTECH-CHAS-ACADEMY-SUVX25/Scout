# Changelog

## 2026-06-10 — UI-simulator, introskärm och ny UI-arkitektur

### Simulator (`sim/`)
- Ny SDL2-baserad PC-simulator som kompilerar exakt samma UI-kod som skärmen,
  så att layout och interaktion kan testas utan hårdvara
- Simulatorn har egen `lv_conf.h` och `display.h` som vinner över enhetens
  vid include-sökningen; joystick styrs med mus, wifi-nivå och intro kan simuleras

### Introskärm
- Laddningsöverlägg vid uppstart: SCOUT-logga i 96px pixelfont, förloppsindikator
  med gradient, statusrad och procentvisning
- Visar de riktiga init-stegen (WIFI, MONITOR, STREAM, READY) medan respektive
  modul startar; varje steg renderas direkt eftersom render-loopen inte kör än
- Överlägget stänger sig självt en stund efter sista steget

### UI-design
- Omstylad till mörkt tema med cyan accent: flytande kort med rundade hörn,
  crosshatch-textur på panelerna och pixelfont (Press Start 2P) genomgående
- Topbar med brand och wifi-symbol (punkt + tre bågar som tänds efter signalnivå),
  bottenbar med nätverksfakta
- Kommandobadges (FWD/BWD/STP/LFT/RGT) flyttade ovanför joysticken och tänds
  efter aktivt kommando; knopp och halo håller sig inom joystickramen
- Telemetripanel med rader för temperatur, luftfuktighet och tryck

### Arkitektur
- UI-koden utbruten ur `lvgl_port` till en egen komponent `components/ui`
  (`scout_ui.c/h`); publika funktioner bytte prefix till `scout_ui_*`
- Fontfilerna flyttade till `components/ui/fonts/`
- `lvgl_port` är nu en ren port (LVGL ↔ display/touch); `main.c` initierar
  UI:t själv efter `lvgl_port_init()`, samma ordning som simulatorn
