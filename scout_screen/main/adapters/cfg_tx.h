#pragma once
#include <stdint.h>

struct sockaddr_in;

void cfg_tx_init(void);
void cfg_tx_bind(int sock);
void cfg_tx_learn(const struct sockaddr_in *src);
void cfg_tx_send(uint8_t cmd, int8_t value);

void cfg_tx_push(uint8_t cmd, int8_t value);
void cfg_tx_flush(void);
