#pragma once
#include "screen_state.h"
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

// Number of lines printed by monitor_cmd_stream — used by callers to reposition the cursor.
#define STREAM_LINE_COUNT 12

// Prints the STREAM stats block (STREAM_LINE_COUNT lines).
void monitor_cmd_stream(const screen_state_t *s);

// Routes line to the matching command handler and prints the result.
// STREAM is not handled here — callers must detect it and call monitor_cmd_stream directly.
void monitor_dispatch(const char           *line,
                      const monitor_status_t *status,
                      const monitor_diag_t   *diag);
