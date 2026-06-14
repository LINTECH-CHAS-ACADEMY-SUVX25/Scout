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

// Mutually exclusive UI modes ("scenes"). Any task on any core signals a mode
// change the same way: screen_state_set_scene(...). Widget reactions happen only
// in the render task via scene_render (LVGL is core 1-only and not thread-safe).
typedef enum {
    SCENE_BOOTING,        // initial state, before the stream task has reported anything
    SCENE_WAITING,        // no frame has ever arrived — waiting for the cam to show up
    SCENE_STREAMING,      // frames are arriving
    SCENE_DISCONNECTED,   // frames stopped and the cam dropped off the AP
    SCENE_COUNT,
} scene_t;

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

// Sets the active scene. Callable from any task/core — lock-free single-word
// store, last writer wins. Logs each transition once.
void screen_state_set_scene(scene_t s);

// Returns the active scene. Read by scene_render each render tick.
scene_t screen_state_get_scene(void);

// Returns a short lowercase name for s — for logs and the UART monitor.
const char *screen_state_scene_name(scene_t s);

// Returns true if a full frame was received within the last 2 seconds.
bool screen_state_is_streaming(void);

// Returns true once any full frame has been received since boot.
bool screen_state_has_streamed(void);

// Copies the latest metrics snapshot into out.
void screen_state_get(screen_state_t *out);

// Diagnostics received from the cam — the screen's view of the remote node.
// set_cam is called by the cam_diag receiver task; get_cam by readers (CAMDIAG).
// Zero until the first packet arrives.
void screen_state_set_cam(const cam_diag_pkt_t *pkt);
void screen_state_get_cam(cam_diag_pkt_t *out);
