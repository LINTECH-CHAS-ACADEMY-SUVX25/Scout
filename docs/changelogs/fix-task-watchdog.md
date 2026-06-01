# fix/task-watchdog

Closes #27

## Ändrat

- `motor_task.c` — registrerar sig med TWDT via `esp_task_wdt_add()`, kallar `esp_task_wdt_reset()` varje loop-iteration
- `network_task.c` — samma som ovan, reset sker i toppen av state-maskinens loop
- TWDT är ännu ej aktiverat i `sdkconfig.defaults` — aktiveras när beteendet verifierats på hårdvaran
