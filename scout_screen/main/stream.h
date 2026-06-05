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

void stream_init(void);
void stream_send_cmd(uint8_t cmd);
bool stream_is_connected(void);
bool stream_try_decode(uint8_t *out_buf, size_t out_size);
void stream_get_stats(stream_stats_t *out);
