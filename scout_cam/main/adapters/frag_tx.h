#pragma once
#include <stdint.h>

// Stores the destination address internally. Must be called once before frag_tx().
void frag_tx_init(const char *ip, uint16_t port);

// Fragments a complete JPEG frame and sends each piece as a separate UDP datagram.
void frag_tx(int sock, const uint8_t *buf, uint32_t len, uint16_t seq);
