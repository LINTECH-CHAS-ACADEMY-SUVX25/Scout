#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "rc_protocol.h"

// State hub for scout_cam — written by the stream task, read by any task that needs cam status.
typedef struct {
    bool   streaming;       // stream is active (not paused)
    bool   screen_online;   // screen is currently sending commands
    bool   camera_enabled;  // false = skip capture and video TX (user-commanded off)
    bool   sensor_enabled;  // false = skip BME280 reads and sensor fields in diag
    int8_t rssi_dbm;        // WiFi RSSI in dBm; updated by cam_state_update_rssi()
} cam_status_t;

extern cam_status_t cam_status;

// Sets camera_enabled and sensor_enabled to true. Call once before cam_state_camera_start.
void cam_state_init(void);

// Initialises the camera with retry + backoff. On repeated failure calls esp_restart() —
// the caller never sees a false return.
bool cam_state_camera_start(void);

// Reads the current WiFi RSSI and stores it in cam_status.rssi_dbm.
void cam_state_update_rssi(void);

// Called from stream_run when cam_status.streaming is false.
// Checks for WiFi reconnect and sets cam_status.streaming = true if reconnected.
void cam_state_try_resume(void);

// Applies a control command received from scout_screen on CTRL_PORT.
// Handles ON/OFF commands directly; delegates SET_* commands to camera_apply_setting.
void cam_state_apply_ctrl(const cam_ctrl_pkt_t *pkt);
