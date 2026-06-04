#pragma once
#include <stdint.h>
#include <stddef.h>
#include "lwip/sockets.h"

// Thin UDP datagram-socket helpers shared by the cam (sender) and screen
// (receiver). Keeps the raw socket calls out of the application code.

// Opens a UDP socket. If bind_port != 0 the socket is bound to that local port
// for receiving; pass 0 for a send-only socket. Returns the fd, or -1 on error.
int  udp_open(uint16_t bind_port);
void udp_close(int sock);

// Best-effort tuning of the kernel receive buffer.
void udp_set_rcvbuf(int sock, int bytes);

// Blocking receive of one datagram. Returns bytes read, or <0 on error.
// src may be NULL when the sender address isn't needed.
int  udp_recv(int sock, void *buf, size_t len, struct sockaddr_in *src);

// Non-blocking receive. Returns bytes read, 0 when nothing is waiting, <0 on error.
int  udp_try_recv(int sock, void *buf, size_t len);

// Fire-and-forget send of one datagram to dst.
void udp_send(int sock, const struct sockaddr_in *dst, const void *buf, size_t len);

// Builds an address from a dotted-quad IPv4 string and port.
struct sockaddr_in udp_addr(const char *ip, uint16_t port);
