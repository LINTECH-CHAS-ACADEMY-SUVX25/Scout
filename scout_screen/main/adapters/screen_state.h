#pragma once
#include "ring_buffer.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t     ms;
    bool        done;
    bool        is_frame;   // if set, tick commits disp interval and increments frame_count
    ring_buf_t *ring;
} tick_slot_t;

typedef struct {
    int64_t     tick_start_us;
    int64_t     split_start_us;
    tick_slot_t lvgl;
    tick_slot_t decode;
    tick_slot_t blit;
    tick_slot_t transfer;
} screen_tick_t;

// Connection and liveness state — written by stream_run, read by render_run.
typedef struct {
    bool cam_connected;  // camera is associated with the AP
    bool streaming;      // frames are actively arriving
} screen_status_t;

// Performance metrics snapshot returned by screen_state_get.
typedef struct {
    uint32_t    frame_count;
    stat_snap_t loop;
    stat_snap_t decode;
    stat_snap_t blit;
    stat_snap_t lvgl;
    stat_snap_t disp;
    stat_snap_t transfer;
    stat_snap_t rx_interval;
    stat_snap_t frame_bytes;
    uint32_t    rx_fps_tenths;
    uint32_t    disp_fps_tenths;
} screen_state_t;

// Updated by stream_run each loop iteration.
extern screen_status_t screen_status;

// Called by stream_run after a complete frame is received; now_ms is the receive timestamp.
void screen_state_push_transfer(uint32_t now_ms, uint32_t len, int32_t transfer_ms);

// Wires ring pointers and flags into ctx for the render task.
// Call once before render_run's loop starts.
void screen_state_render_tick_init(screen_tick_t *ctx);

// Call at the top of any task loop. Commits the previous iteration's splits into the
// ring buffers and starts a fresh timer. Records full loop time (tick-to-tick).
void screen_state_tick(screen_tick_t *ctx);

// Records elapsed time since the previous split (or tick) into slot.
// Pass a pointer to the named field: &ctx->lvgl, &ctx->decode, etc.
void screen_state_tick_split(screen_tick_t *ctx, tick_slot_t *slot);

// Copies the latest metrics snapshot into out.
void screen_state_get(screen_state_t *out);
