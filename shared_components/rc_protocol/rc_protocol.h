#pragma once
#include <stdint.h>

// Network identity
#define AP_SSID     "Scout_AP"
#define AP_PASS     "scout1234"
#define S3_IP       "192.168.4.1"
#define VID_PORT    3334
#define CMD_PORT    3335

// Motor commands (1-byte values sent from screen to cam over CMD_PORT)
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
