# feature/uart-monitor-tests

Lägger till ett automatiserat testskript för scout_screen UART-monitorn. Testat mot hårdvara: 20/20 checks passed.

## testskript (`tests/`)

- Lade till `tests/test_uart_monitor.py` — pyserial-skript som skickar STATUS, STREAM, DIAG, HELP och ett ogiltigt kommando mot scout_screen via USB serial
- Verifierar att varje svar innehåller förväntade fält och headers
- Skickar `q` för att avsluta STREAM live-läget utan manuell interaktion
- Exits med kod 0 om alla checks passar, 1 annars
- Lade till `tests/requirements.txt` med `pyserial>=3.5`
