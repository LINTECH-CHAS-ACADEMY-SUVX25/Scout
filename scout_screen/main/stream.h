#pragma once

// Allocates frame buffers and the command mutex.
void stream_init(void);

// Passed directly to xTaskCreatePinnedToCore — do not call from application code.
void stream_run(void *arg);
