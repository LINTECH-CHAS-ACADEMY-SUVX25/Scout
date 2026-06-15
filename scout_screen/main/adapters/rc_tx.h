#pragma once
#include <stdint.h>
#include "lwip/sockets.h"

// Creates the address mutex. Call from stream_init before stream_run starts.
void rc_tx_init(void);

// Binds the socket used for outbound commands. Call from stream_run after udp_open.
void rc_tx_bind(int sock);

// Records the camera's source address from the first received packet. No-op thereafter.
void rc_tx_learn(const struct sockaddr_in *src);

// Sends a joy_pkt_t to the camera. Safe to call before the camera is known.
void rc_tx_send(int16_t x, int16_t y);

// Sends immediately when either axis changes by more than 5 units; repeats every
// 200 ms as a keepalive. Call once per render loop iteration.
void rc_tx_send_throttled(int16_t x, int16_t y);
