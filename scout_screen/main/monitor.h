#pragma once

// Initialises the UART console.
void monitor_init(void);
// Passed directly to xTaskCreate — do not call from application code.
void monitor_run(void *arg);
