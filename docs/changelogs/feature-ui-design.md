# feature/ui-design

Designpass på huvudlayouten mot en premium tech-känsla: hela UI:t är nu
flytande rundade kort kring den centrerade kameran. Bygger på
feature/loading-screen — all UI-kod ligger i delade `lvgl_port_ui.c`.

## Tillagt

- Wifi-symbol i topbaren (prick + tre bågar, mobilstil) som ersätter
  "waiting..."/"connected"-texten. `lvgl_port_ui_update(nivå)` tar 0-3:
  0 = ingen länk (röd prick), 1-3 tänder bågarna inifrån och ut
- Viewfinder-hörn i accentfärg runt kameraytan — ritas av delade UI:t och
  syns även på enheten; videon blittas mellan dem och disconnect-rensningen
  rör dem inte
- Kryssmönstertextur på hörnpanelerna: en 8×8-tile byggs vid init
  (linjefärg med låg alpha, diagonaler i båda riktningarna) och tilas som
  bakgrundsbild så den maskas av hörnradien. Rattar: `STRIPE_TILE`,
  `STRIPE_W`, `STRIPE_OPA`
- Hjälpare: `make_panel()` (delad kortstil), `make_vsep()`,
  `make_wifi_arc()`, `add_stripes()`

## Ändrat

- Layouten omgjord till flytande kort med 8 px luft mot skärmkanterna:
  topbar och bottombar rundade, telemetrin i egen panel uppe till höger,
  joysticken i egen panel nere till vänster. Panelerna är lika stora och
  linjerar med kamerafönstrets över- respektive underkant
- Telemetriraderna i ett upphöjt inre kort med radavskiljare, samma
  bakgrund som barerna; värdena i cyan
- Badges pillformade, joysticken fick en yttre koncentrisk ring,
  rubrikstrecken vertikalt centrerade mot texten
- FREERTOS i bottombaren i cyan; loggan utan markering framför;
  hårlinjeavdelare mellan grupperna i båda barerna
- `lvgl_port_ui_init` uppdelad i en byggfunktion per modul —
  `make_topbar`, `make_botbar`, `make_tele_panel`, `make_joy_panel`,
  `make_joystick`, `make_cmd_badges`, `make_cam_corners` — så varje del
  är lätt att hitta och ändra. Pixeldiffad mot före: identisk
- Simulatorn: **c** stegar wifi-nivån 0-3 i stället för att växla
  connected-state

## Borttaget

- "waiting..."/"connected"-etiketten och statuspricken i topbaren
- Sidofältet i helhöjd och dess divider — ersatta av hörnpanelerna

## Verifierat

- Skärmdumpar i simulatorn efter varje steg, uppdelningen av init
  pixeldiffad mot före (identisk), `idf.py build` ok

## Kvarstår

- Wifi-symbolen matas inte med riktig signalstyrka än — render.c skickar
  i dag 0/1 utifrån stream-connected. RSSI-mappningen (t.ex. via
  AP:ns stationslista) implementeras separat; kontraktet är
  `lvgl_port_ui_update(0-3)`
