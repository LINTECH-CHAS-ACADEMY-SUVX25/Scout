#pragma once
#include <stdint.h>
#include "lwip/sockets.h"

void rc_tx_init(void);
void rc_tx_bind(int sock);

/**
 * @brief  Records the camera's source address from the first received packet. No-op thereafter.
 * @param  src  Source address extracted from the received UDP packet.
 */
void rc_tx_learn(const struct sockaddr_in *src);

/**
 * @brief  Sends a joy_pkt_t to the camera. Safe to call before the camera's address is known.
 * @param  x  Horizontal axis (-255..255, positive = right).
 * @param  y  Vertical axis (-255..255, positive = forward).
 */
void rc_tx_send(int16_t x, int16_t y);

/**
 * @brief  Sends immediately when either axis changes by more than 5 units;
 *         repeats every 200 ms as a keepalive.
 * @param  x  Horizontal axis (-255..255, positive = right).
 * @param  y  Vertical axis (-255..255, positive = forward).
 * @note   Call once per render loop iteration.
 */
void rc_tx_send_throttled(int16_t x, int16_t y);
