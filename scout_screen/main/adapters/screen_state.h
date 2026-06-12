#pragma once
#include "ring_buffer.h"
#include "rc_protocol.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t     ms;
    bool        done;
    bool        counts_frame;        // if set, increments frame_count when committed
    bool        updates_streaming;   // if set, updates the liveness timestamp in screen_state
    ring_buf_t *ring;                // target ring; NULL = slot inactive
    ring_buf_t *interval_ring;       // if set, records time between consecutive done events
    uint32_t    prev_interval_ms;    // internal state — do not set manually
} tick_slot_t;

typedef struct {
    int64_t     tick_start_us;
    int64_t     split_start_us;
    ring_buf_t *loop_ring;           // target ring for loop time; NULL = don't record
    tick_slot_t lvgl;
    tick_slot_t decode;
    tick_slot_t blit;
    tick_slot_t transfer;
    tick_slot_t bytes;
} screen_tick_t;

// Connection and liveness state — written by stream_run, read by render_run.
typedef struct {
    bool  cam_connected;  // camera is associated with the AP
    bool  streaming;      // frames are actively arriving
} screen_status_t;

// Performance metrics snapshot returned by screen_state_get.
typedef struct {
    uint32_t    frame_count;
    stat_snap_t render_loop;
    stat_snap_t stream_loop;
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

// Wires ring pointers and flags into ctx for the render task.
// Call once before render_run's loop starts.
void screen_state_render_tick_init(screen_tick_t *ctx);

// Wires ctx for the stream task.
// Call once before stream_run's loop starts.
void screen_state_stream_tick_init(screen_tick_t *ctx);

// Call at the top of any task loop. Commits the previous iteration's splits into the
// ring buffers and starts a fresh timer. Records full loop time (tick-to-tick).
void screen_state_tick(screen_tick_t *ctx);

// Records elapsed time since the previous split (or tick) into slot.
// Pass a pointer to the named field: &ctx->lvgl, &ctx->decode, etc.
void screen_state_tick_split(screen_tick_t *ctx, tick_slot_t *slot);

// Returns true if a full frame was received within the last 2 seconds.
bool screen_state_is_streaming(void);

// Copies the latest metrics snapshot into out.
void screen_state_get(screen_state_t *out);

// Diagnostics received from the cam — the screen's view of the remote node.
// set_cam is called by the cam_diag receiver task; get_cam by readers (CAMDIAG).
// Zero until the first packet arrives.
void screen_state_set_cam(const cam_diag_pkt_t *pkt);
void screen_state_get_cam(cam_diag_pkt_t *out);
