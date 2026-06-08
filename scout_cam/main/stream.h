#pragma once

// Sets the destination address and logs the configured ports. Call before starting the stream task.
void stream_init(void);

// Passed directly to xTaskCreate — do not call from application code.
void stream_run(void *arg);
