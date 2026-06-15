#pragma once
#include "rc_protocol.h"
#include <stdint.h>
#include <stdbool.h>

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

// Updated by stream_run each loop iteration.
extern screen_status_t screen_status;

// Called by screen_stats when a transfer slot commits to keep liveness current.
void screen_state_mark_rx_time(uint32_t now_ms);

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

// Diagnostics received from the cam — the screen's view of the remote node.
// set_cam is called by stream_run; get_cam by readers (CAMDIAG, render).
// Zero until the first packet arrives.
void screen_state_set_cam(const cam_diag_pkt_t *pkt);
void screen_state_get_cam(cam_diag_pkt_t *out);

// Returns true and clears the flag if a new cam packet arrived since the last
// call. Lock-free; called by render to refresh the telemetry readouts on arrival.
bool screen_state_cam_dirty_take(void);
