# fix/simplify-codebase-screen — uppföljning 2026-06-04

## Tillagt
- `sdkconfig.defaults` — hårdvaruspecifika inställningar tillagda (flash-storlek, OPI PSRAM, CPU-frekvens, kompilatoroptimering, loggning) så att partnerns build får samma konfiguration automatiskt

## Ändrat
- `lv_conf.h` — aktiverat (`LV_CONF_SKIP=n`), LVGL använder nu faktiskt projektets inställningar istället för Kconfig-defaults
- `lv_conf.h` — minneshantering ändrad från statisk pool (`LV_MEM_CUSTOM=0`, 128KB i intern SRAM) till `malloc` (`LV_MEM_CUSTOM=1`) — LVGL-heapen går nu till PSRAM via `SPIRAM_USE_MALLOC`, vilket frigör intern SRAM för bounce buffer och WiFi-stacken
- `components/lvgl__lvgl` — LVGL flyttad från `managed_components/` till `components/` för att hålla buggfixen spårad i git
- `components/lvgl__lvgl/env_support/cmake/esp.cmake` — `main` tillagd i REQUIRES så att `lv_conf_internal.h` hittar `lv_conf.h` (IDF 6.1 kräver explicit beroende)
- `firmware/screen/main/idf_component.yml` — `lvgl/lvgl` borttagen som managed dependency eftersom LVGL nu är en lokal komponent

HUMAN WARNING:

Jag har inte användt lv_conf eftersom det inte varit igång i idf.py menuconfig, fan va klantigt

sen tror jag inte ni heller gjort det eftersom det första som hände när jag aktivera det var att skärmen kraschade flera gånger >:)

//alex