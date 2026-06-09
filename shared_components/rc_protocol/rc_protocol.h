#pragma once

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

// UDP video fragmentation — shared so cam and screen never drift apart
#define FRAME_MAGIC  0xAB
#define FRAG_SIZE    1460
#define FRAG0_HDR    5                       // magic(1) + frame_len(4) in fragment 0
#define FIRST_DATA   (FRAG_SIZE - FRAG0_HDR)
#define FRAME_MAX    (32 * 1024)
#define MAX_FRAGS    (1 + (FRAME_MAX - FIRST_DATA + FRAG_SIZE - 1) / FRAG_SIZE)
#define PKT_MAX      (4 + FRAG0_HDR + FRAG_SIZE)
