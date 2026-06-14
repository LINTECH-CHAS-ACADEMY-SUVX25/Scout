# feature/loading-screen

Intro-skärmen visar nu riktig boot-progress i stället för en tidsstyrd
animation, med större logga och en modernare laddningsbar. Bygger på
feature/ui-sim — all UI-kod ligger i den delade `lvgl_port_ui.c`.

## Tillagt

- `lvgl_port_intro_step(label)` — `app_main` anropar ett steg per init
  (WIFI, MONITOR, STREAM, READY); baren fylls per steg och statustexten
  visar vad som faktiskt initieras just då
- Procentsiffra till höger under baren (25/50/75/100 %)
- `press_start_2p_96.c` — loggan i 96 px. Genererad med enbart mellanslag
  + A–Z; full ASCII i 96 px hade kostat ~110 KB flash för fem använda
  bokstäver
- `COL_ACCENT_DEEP` i paletten — gradientens startfärg

## Ändrat

- `lvgl_port_intro_screen()` tar antal steg som argument
- Varje steg renderas direkt via `lv_refr_now()` — under boot kör
  render-loopen inte än, så UI-koden måste själv få ut framen på skärmen
- `main.c` omordnad: display + LVGL initieras före wifi/monitor/stream så
  att stegen syns medan de kör. Render-tasken startar sist som tidigare —
  den kräver att stream redan är initierad (frame_buf/cam_cmd)
- Laddningsbaren omgjord: smal rundad track (440×14, 1 px kant) med
  gradientfyllning djup cyan → accent, i stället för 400×22-lådan med
  heltäckande fyllning
- Overlayn stängs av en engångstimer 1,2 s efter sista steget — loggan
  och den färdiga baren försvinner i samma frame
- Rubrikstrecken (TELEMETRI/JOYSTICK) vertikalt centrerade mot texten
- `sim/README.md` uppdaterad efter nya intron

## Borttaget

- Den fasta 2,5 s-animationen och den hårdkodade "LOADING..."-texten
- `press_start_2p_24.c` ur byggena — loggan använder 96-varianten.
  Filen ligger kvar i repot om mindre storlek behövs igen

## Verifierat

- Skärmdumpar i simulatorn mitt i intron, under hold-fönstret och efter
  stängning — loggan står kvar hela hold-tiden
- `idf.py build` ok, 44 % fritt i app-partitionen trots 96 px-fonten

## Kvarstår

- På enheten styrs stegtempot av riktiga init-tider — bootar systemet
  fort går intron fort. Medvetet vald avvägning: inga konstgjorda
  fördröjningar i `app_main`, en spinnvänta där riskerar idle-watchdogen
