# feature/task-architecture

## Tillagt
- `motor_task.c/.h` — FreeRTOS-task som hanterar motorkommandon via kö, nödstopp efter 500 ms utan kommando
- `network_task.c/.h` — TCP-streaming med explicit state machine (DISCONNECTED → CONNECTING → STREAMING), socket-timeout på send

## Ändrat
- `main.c` — startar `motor_task` och `network_task` istället för `stream_start()`
- `CMakeLists.txt` — uppdaterad med nya källfiler
- `rc_protocol.h` — `S3_IP` flyttad hit från stream.c

## Borttaget
- `stream.c/.h` — ersatt av `motor_task` och `network_task`
