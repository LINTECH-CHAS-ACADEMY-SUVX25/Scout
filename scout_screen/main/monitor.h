#pragma once

// Initialises the UART console and starts the diagnostic monitor task.
//
// Available commands (115200 baud, case-sensitive):
//   STATUS  — uptime, free heap, PSRAM, WiFi clients, stream connection
//   STREAM  — frame count, last frame size, transfer and decode timing
//   DIAG    — heap watermarks, task count
//   HELP    — list all commands
void monitor_init(void);
