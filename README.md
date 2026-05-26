# Scout — WiFi-styrd RC-bil med live-telemetri

RC-bil styrd via en 7-tums pekskärm över WiFi. Skärmen visar live-video från bilen och tar emot styrkommandon via en virtuell joystick. Systemet körs på två ESP32-noder som kommunicerar via TCP.

## Hårdvara

| Komponent | Roll |
|---|---|
| ESP32-S3-Touch-LCD-7B | Dashboard — pekskärm, joystick, video |
| ESP32-CAM (AI-Thinker) | Bil — kamera, motorstyrning, telemetri |
| L298N H-brygga | Motorförare — styr de två DC-motorerna |
| Robot Car Kit 2WD | Chassi med motorer och hjul |

## Nätverkstopologi

Skärmen (ESP32-S3) startar ett WiFi AP (`Scout_AP`). Bilen (ESP32-CAM) ansluter som STA-klient. All kommunikation sker via TCP på port `3334`.

```
[Pekskärm / AP]  <──TCP──>  [Bil / STA]
  Skickar: styrkommandon (1 byte)
  Tar emot: JPEG-videoframes
```

## Repo-struktur

```
firmware/
  cam/        — ESP32-CAM firmware (bil-sidan)
  screen/     — ESP32-S3 firmware (skärm-sidan)
components/
  rc_protocol/  — Delat kommunikationsprotokoll
  gpio/         — GPIO-abstraktion
  i2c/          — I2C-drivrutin
  touch/        — GT911 pekskärmscontroller
  rgb_lcd_port/ — Waveshare LCD-drivrutin
docs/
  implementation/plan.md          — Implementationsplan (7 faser)
  RC_Bil_Boiler_Room_Projekt.md   — Projektkrav och kursplan
  changelogs/                     — Changelog per branch
```

## Bygga och flasha

Kräver [ESP-IDF v6.0.1](https://github.com/espressif/esp-idf/tree/v6.0.1).

**Bil-sidan (ESP32-CAM):**
```bash
cd firmware/cam
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

**Skärm-sidan (ESP32-S3):**
```bash
cd firmware/screen
idf.py build
idf.py -p /dev/ttyUSB1 flash monitor
```

Avsluta monitor med `Ctrl+T` → `Ctrl+X`.

## Starta systemet

1. Flasha och starta skärmen — den startar WiFi AP:t
2. Flasha och starta bilen — den ansluter automatiskt till AP:t
3. När bilen anslutit visas live-video på skärmen och joysticken är aktiv

## Bidra

Se [CONTRIBUTING.md](CONTRIBUTING.md) för branch-namngivning, commit-stil och PR-flöde.
