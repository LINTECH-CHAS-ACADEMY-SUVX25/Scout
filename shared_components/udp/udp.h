#pragma once
#include <stdint.h>
#include <stddef.h>
#include "lwip/sockets.h"

// Thin UDP datagram helpers — keeps raw socket calls out of application code.

// Opens a UDP socket. bind_port 0 = send-only. Returns fd or -1 on failure.
int  udp_open(uint16_t bind_port);
void udp_close(int sock);
void udp_set_rcvbuf(int sock, int bytes);

// Blocking receive. src may be NULL if the source address is not needed.
int  udp_rx(int sock, void *buf, size_t len, struct sockaddr_in *src);

// Non-blocking receive. Returns 0 if nothing is waiting, -1 on error.
int  udp_try_recv(int sock, void *buf, size_t len);

void udp_tx(int sock, const struct sockaddr_in *dst, const void *buf, size_t len);

struct sockaddr_in udp_addr(const char *ip, uint16_t port);

// Returns a static string with the dotted-decimal IP of addr.
const char *udp_ip_str(const struct sockaddr_in *addr);
