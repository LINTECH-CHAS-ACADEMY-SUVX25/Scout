#pragma once

#define AP_SSID      "Scout_AP"
#define AP_PASS      "scout1234"

#define S3_IP        "192.168.4.1"
#define VID_PORT     3334
#define CMD_PORT     3335
#define FRAME_MAGIC  0xAB

// UDP-videofragmentering — delas så cam och screen aldrig glider isär
#define FRAG_SIZE    1460
#define FRAG0_HDR    5                       // magic + frame_len i fragment 0
#define FIRST_DATA   (FRAG_SIZE - FRAG0_HDR)
#define FRAME_MAX    (32 * 1024)             // största JPEG mottagaren buffrar
#define MAX_FRAGS    (1 + (FRAME_MAX - FIRST_DATA + FRAG_SIZE - 1) / FRAG_SIZE)
#define PKT_MAX      (4 + FRAG0_HDR + FRAG_SIZE)

#define CMD_STOP     0x00
#define CMD_FORWARD  0x01
#define CMD_BACKWARD 0x02
#define CMD_LEFT     0x04
#define CMD_RIGHT    0x08
