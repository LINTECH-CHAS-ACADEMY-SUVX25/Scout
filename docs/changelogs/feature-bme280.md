# feature/bme280

BME280-miljösensor inkopplad på cam-sidan via I2C (SDA=GPIO2, SCL=GPIO3, adress 0x77,
100 kHz). Sensorvärdena fyller nu i de tidigare nollstubbade fälten i `cam_diag_pkt_t`,
så temperatur/luftfuktighet/lufttryck syns via CAMDIAG i screen-sidans UART-monitor.
Saknad/trasig sensor är inte fatal: cam fortsätter streama och fälten är noll.
**Verifierad på enhet** (CAMDIAG visar ~27 °C / 36 % / 100220 Pa).

## Nytt

- `scout_cam/components/bme280/` (ny komponent) — handskriven Bosch BME280-drivrutin över
  nya `driver/i2c_master.h`-API:t. Exponerar `bme280_init()` och
  `bme280_read(float *temp, float *hum, float *pres)`. SDA=GPIO2, SCL=GPIO3, adress 0x77,
  100 kHz, interna pull-ups. Forced-mode-mätning med oversampling x1; flyttalskompensation
  enligt databladet (sektion 4.2.3). Chip-id-kontroll (0x60) vid init med I2C-busscan i
  felsökvägen — misslyckas den loggas en varning och sensorn lämnas inaktiv (ingen
  boot-loop utan sensor).

## Ändrat

- `scout_cam/main/telemetry.c` — anropar `bme280_read()` per tick; vid lyckad läsning
  konverteras flyttalen till paketens enheter (`temp_cdeg` = °C×100, `humidity_pct` = %,
  `pressure_pa` = Pa)

- `scout_cam/main/main.c` — anropar `bme280_init()` mellan `camera_init()` och
  `stream_init()`

- `scout_cam/main/CMakeLists.txt` — lade till `bme280` i REQUIRES
