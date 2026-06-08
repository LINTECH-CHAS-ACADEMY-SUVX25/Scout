#pragma once
#include "ICamera.hpp"
#include <stdint.h>

typedef struct {
    uint32_t frames_sent;
    uint32_t last_frame_bytes;
} udp_stream_stats_t;

// Starts the UDP stream task: captures JPEG frames via ICamera, fragments them,
// and sends them to the screen. Also drains inbound CMD bytes and forwards them
// to the motor task. udp_stream never includes camera.h — it only knows ICamera.
void udp_stream_start(ICamera *camera);
void udp_stream_get_stats(udp_stream_stats_t *out);
