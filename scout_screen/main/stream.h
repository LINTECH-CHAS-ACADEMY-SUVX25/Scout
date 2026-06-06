#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint32_t frame_count;
    uint32_t last_frame_bytes;
    int32_t  last_transfer_ms;
    int32_t  last_decode_ms;
} stream_stats_t;

// Opens the UDP video socket and starts the receiver task on core 0.
void stream_init(void);

// Sends a 1-byte RC command to the camera's CMD_PORT.
void stream_send_cmd(uint8_t cmd);

// Returns true if a full frame has been received within the last 2 seconds.
bool stream_is_connected(void);

// Decodes the most recently assembled frame into out_buf as RGB565.
// Returns true on success. Call from the render task only.
bool stream_try_decode(uint8_t *out_buf, size_t out_size);

// Copies the latest stream statistics into out.
void stream_get_stats(stream_stats_t *out);
