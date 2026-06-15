#pragma once
#include <stdbool.h>
#include <stdint.h>

// State hub for scout_cam — written by the stream task, read by any task that needs cam status.
typedef struct {
    bool   streaming;     // stream is active (not paused)
    bool   screen_online; // screen is currently sending commands
    bool   camera_fault;  // camera init failed for good — node runs degraded (no video)
    int8_t rssi_dbm;      // WiFi RSSI in dBm; updated by cam_state_update_rssi()
} cam_status_t;

extern cam_status_t cam_status;

// Initialises the camera with retry + backoff (the driver reports failures, this
// function owns the policy). On repeated failure sets cam_status.camera_fault and
// returns false — the node keeps running degraded (motors + telemetry, no video)
// instead of reboot-looping.
bool cam_state_camera_start(void);

// Reads the current WiFi RSSI and stores it in cam_status.rssi_dbm.
void cam_state_update_rssi(void);

// Called from stream_run when cam_status.streaming is false.
// Checks for WiFi reconnect or incoming command; sets cam_status.streaming = true if either occurs.
void cam_state_try_resume(int sock);

// Called from stream_run after each frame transmission.
// Drains pending CMD bytes and forwards them to the motor task.
// Sets cam_status.streaming = false after too many consecutive silent frames.
void cam_state_process_cmds(int sock);
