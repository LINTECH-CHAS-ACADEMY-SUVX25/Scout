#pragma once
#include <stdint.h>

typedef struct {
    uint32_t frames_sent;
    uint32_t last_frame_bytes;
} udp_stream_stats_t;

void udp_stream_start(void);
void udp_stream_get_stats(udp_stream_stats_t *out);
