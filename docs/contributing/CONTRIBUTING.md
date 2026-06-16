# Contributing

## Branches

Skapa alltid en branch från `main` och namnge den efter vad du jobbar med:

```
feature/uart-diagnostics
fix/motor-stop-on-disconnect
docs/architecture-diagrams
```

Pusha aldrig direkt till `main`. Allt går via pull request.

## Issues

Skapa en issue innan du börjar med något icke-trivialt. Håll titeln kort och beskriv problemet, inte lösningen. Hänvisa till relaterade issues med `#nummer` om det finns kopplingar.

## Changelog

Innan varje commit, uppdatera eller skapa en fil i `docs/changelogs/` som beskriver vad du gjort. Namnge filen efter din branch:

```
docs/changelogs/feature-uart-diagnostics.md
docs/changelogs/fix-motor-stop-on-disconnect.md
```

Håll det kort — vad lades till, vad ändrades, vad togs bort. Ingen roman.

## Commits

Skriv vad du faktiskt ändrade, inte varför. Exempel:

```
Add motor_task with queue-based command handling
Fix socket timeout missing on send_all
```

Undvik vaga meddelanden som `fix`, `update` eller `changes`.

## Pull requests

- En PR per issue
- Länka till issuen i PR-beskrivningen (`Closes #5`)
- Koden ska bygga utan fel innan du begär review
- Minst en annan person granskar innan merge

## Kodbas

Projektet är ett ESP-IDF-projekt uppdelat i två firmware-targets:

```
firmware/cam/    — ESP32-CAM (bil-sidan)
firmware/screen/ — ESP32-S3-Touch-LCD-7B (skärm-sidan)
components/      — delad kod, t.ex. rc_protocol
```

Bygga och flasha:

```bash
cd firmware/cam
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Samma sak för `firmware/screen`, fast med rätt port för din S3.
