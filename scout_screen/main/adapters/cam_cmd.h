#pragma once
#include <stdint.h>
#include "lwip/sockets.h"

// Creates the address mutex. Call from stream_init before stream_run starts.
void cam_cmd_init(void);

// Binds the socket used for outbound commands. Call from stream_run after udp_open.
void cam_cmd_bind(int sock);

// Records the camera's source address from the first received packet. No-op thereafter.
void cam_cmd_learn(const struct sockaddr_in *src);

// Sends a 1-byte RC command to the camera. Safe to call before the camera is known.
void cam_cmd_send(uint8_t cmd);

// Sends cmd immediately on change; repeats every 200 ms if unchanged (keepalive).
// Manages last-command state internally — call once per render loop iteration.
void cam_cmd_send_throttled(uint8_t cmd);
