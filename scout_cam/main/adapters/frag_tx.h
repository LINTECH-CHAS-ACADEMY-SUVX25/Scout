#pragma once
#include <stdint.h>

/** @brief Stores the destination address. Must be called once before frag_tx(). */
void frag_tx_init(const char *ip, uint16_t port);

void frag_tx(int sock, const uint8_t *buf, uint32_t len, uint16_t seq);
