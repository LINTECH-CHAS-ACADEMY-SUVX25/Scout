# feature/wire-ui-connections

## Ändrat

- `ui.c`: Ersatt hårdkodad IP `192.168.1.42` med `S3_IP` från `rc_protocol.h`
- `ui.c`: Ersatt hårdkodat port `4210` med `VID_PORT` från `rc_protocol.h`
- `ui.c`: Label ändrad från "UDP" till "TCP" (protokollet är TCP)
- `ui.c`: Lade till `XSTR`/`STR`-makron för att stringifiera `VID_PORT` vid kompilering
