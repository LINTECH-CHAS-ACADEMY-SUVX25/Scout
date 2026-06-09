#pragma once
#include "frame_buf.h"
#include <stdint.h>
#include <stdbool.h>

// Snapshot of system state passed to STATUS command.
typedef struct {
    uint32_t uptime_s;
    uint32_t free_heap;
    uint32_t free_psram;
    int      sta_count;
    bool     stream_connected;
} monitor_status_t;

// Snapshot of diagnostic counters passed to DIAG command.
typedef struct {
    uint32_t n_tasks;
    uint32_t min_heap;
    uint32_t free_int;
    uint32_t free_psram;
} monitor_diag_t;

// Routes line to the matching command handler and prints the result.
void monitor_dispatch(const char           *line,
                      const monitor_status_t *status,
                      const stream_stats_t   *stream,
                      const monitor_diag_t   *diag);
