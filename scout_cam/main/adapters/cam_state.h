#pragma once
#include <stdbool.h>

// State hub for scout_cam — written by the stream task, read by any task that needs cam status.
typedef struct {
    bool streaming;     // stream is active (not paused)
    bool screen_online; // screen is currently sending commands
} cam_status_t;

extern cam_status_t cam_status;

// Called from stream_run when cam_status.streaming is false.
// Checks for WiFi reconnect or incoming command; sets cam_status.streaming = true if either occurs.
void cam_state_try_resume(int sock);

// Called from stream_run after each frame transmission.
// Drains pending CMD bytes and forwards them to the motor task.
// Sets cam_status.streaming = false after too many consecutive silent frames.
void cam_state_process_cmds(int sock);
