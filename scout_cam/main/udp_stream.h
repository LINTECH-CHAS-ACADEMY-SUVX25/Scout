#pragma once

// No-op placeholder — call before starting the stream task.
void udp_stream_init(void);

// Passed directly to xTaskCreate — do not call from application code.
void udp_stream_run(void *arg);
