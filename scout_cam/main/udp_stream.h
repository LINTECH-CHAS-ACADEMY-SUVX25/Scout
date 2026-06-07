#pragma once
#include <stdint.h>

typedef struct {
    uint32_t frames_sent;
    uint32_t last_frame_bytes;
} udp_stream_stats_t;

// Starts the UDP stream task: captures JPEG frames, fragments them, and sends
// them to the screen. Also drains inbound CMD bytes from CMD_PORT and forwards
// them to the motor task.
void udp_stream_start(void);
void udp_stream_get_stats(udp_stream_stats_t *out);
