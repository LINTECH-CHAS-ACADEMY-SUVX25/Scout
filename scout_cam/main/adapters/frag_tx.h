#pragma once
#include <stdint.h>
#include "lwip/sockets.h"

// Fragments a complete JPEG frame and sends each piece as a separate UDP datagram.
void frag_tx(int sock, const struct sockaddr_in *dest,
             const uint8_t *buf, uint32_t len, uint16_t seq);
