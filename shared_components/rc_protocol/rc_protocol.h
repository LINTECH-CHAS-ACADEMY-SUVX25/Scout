#pragma once
#include <stdint.h>

// Network identity
#define AP_SSID     "Scout_AP"
#define AP_PASS     "scout1234"
#define S3_IP       "192.168.4.1"
#define VID_PORT    3334
#define CMD_PORT    3335

// Joystick packet — screen sends this to cam over CMD_PORT (4 bytes)
// x: -255..255, positive = right; y: -255..255, positive = forward
typedef struct __attribute__((packed)) {
    int16_t x;
    int16_t y;
} joy_pkt_t;

// Joystick geometry — shared so screen and cam use identical scaling
#define JOY_RADIUS_PX    34
#define JOY_DEADZONE_PX  10
#define JOY_SPEED_MIN    ((JOY_DEADZONE_PX * 255) / JOY_RADIUS_PX)  // = 75

// Motor direction bitmask — cam-internal; derived from joy_pkt_t on the cam side
#define CMD_STOP     0x00
#define CMD_FORWARD  0x01
#define CMD_BACKWARD 0x02
#define CMD_LEFT     0x04
#define CMD_RIGHT    0x08

// Camera frame resolution — the cam crops the VGA (640x480) sensor frame to a centered
// square via OV2640 windowing before sending (see scout_cam camera.c). Both nodes derive
// buffer sizes and the screen blit region from these constants.
#define CAM_W        480
#define CAM_H        480

// Diagnostics port — cam sends cam_diag_pkt_t to screen every 2 s
#define DIAG_PORT    3336

// Control port — screen sends cam_ctrl_pkt_t to cam to configure the camera or toggle features
#define CTRL_PORT    3337

typedef enum __attribute__((packed)) {
    CAM_CTRL_CAMERA_ON          = 0x01,
    CAM_CTRL_CAMERA_OFF         = 0x02,
    CAM_CTRL_SENSOR_ON          = 0x03,
    CAM_CTRL_SENSOR_OFF         = 0x04,
    CAM_CTRL_SET_QUALITY        = 0x10,  // 0-63 (lower = better JPEG)
    CAM_CTRL_SET_AE_LEVEL       = 0x11,  // 0-10 (0 = re-enable AEC; 1-10 = manual exposure, aec_value=value*100)
    CAM_CTRL_SET_AGC_GAIN       = 0x12,  // 0-10 (0 = re-enable AGC; 1-10 = agc_gain=value*3)
    CAM_CTRL_SET_HMIRROR        = 0x14,  // 0 or 1
    CAM_CTRL_SET_VFLIP          = 0x15,  // 0 or 1
    CAM_CTRL_SET_SPECIAL_EFFECT = 0x16,  // 0-6
} cam_ctrl_cmd_t;

typedef struct __attribute__((packed)) {
    uint8_t cmd;   // cam_ctrl_cmd_t
    int8_t  value; // used by SET_* commands; ignored for ON/OFF
} cam_ctrl_pkt_t;

// Diagnostics packet sent from scout_cam to scout_screen over DIAG_PORT.
// Sensor fields (temp/humidity/pressure) are zero until BME280 is wired (#54).
typedef struct __attribute__((packed)) {
    int16_t  temp_cdeg;     // temperature in 0.01 °C
    uint8_t  humidity_pct;  // relative humidity 0-100 %
    uint32_t pressure_pa;   // atmospheric pressure in Pa
    uint32_t free_heap;     // free heap in bytes
    int8_t   rssi_dbm;      // WiFi RSSI in dBm
    uint32_t uptime_s;      // uptime in seconds
} cam_diag_pkt_t;

// UDP video fragmentation — shared so cam and screen never drift apart
#define FRAME_MAGIC  0xAB
#define FRAG_SIZE    1460
#define FRAG0_HDR    5                       // magic(1) + frame_len(4) in fragment 0
#define FIRST_DATA   (FRAG_SIZE - FRAG0_HDR)
#define FRAME_MAX    (32 * 1024)
#define MAX_FRAGS    (1 + (FRAME_MAX - FIRST_DATA + FRAG_SIZE - 1) / FRAG_SIZE)
#define PKT_MAX      (4 + FRAG0_HDR + FRAG_SIZE)
