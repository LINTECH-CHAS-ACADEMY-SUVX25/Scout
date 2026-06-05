#pragma once
#include <stdint.h>
#include <stddef.h>
#include "lwip/sockets.h"

// Thin UDP datagram helpers — keeps the raw socket calls out of app code.

int  udp_open(uint16_t bind_port);   // bind_port 0 = send-only; returns fd or -1
void udp_close(int sock);
void udp_set_rcvbuf(int sock, int bytes);

int  udp_rx(int sock, void *buf, size_t len, struct sockaddr_in *src);  // blocking; src may be NULL
int  udp_try_recv(int sock, void *buf, size_t len);                     // non-blocking; 0 = nothing waiting
void udp_tx(int sock, const struct sockaddr_in *dst, const void *buf, size_t len);

struct sockaddr_in udp_addr(const char *ip, uint16_t port);
const char        *udp_ip_str(const struct sockaddr_in *addr);
