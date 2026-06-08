#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Installs the UART0 driver and starts the interactive monitor task.
// Commands: STATUS, MOTOR, SENSOR, DIAG, CONFIG, HELP
void monitor_init(void);

#ifdef __cplusplus
}
#endif
