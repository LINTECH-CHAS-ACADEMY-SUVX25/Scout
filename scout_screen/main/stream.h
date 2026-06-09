#pragma once

// Allocates frame buffers, the command mutex, and spawns the stream task on core 0.
void stream_init(void);
