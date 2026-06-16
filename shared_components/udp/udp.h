#pragma once
#include <stdint.h>
#include <stddef.h>
#include "lwip/sockets.h"

// Thin UDP datagram helpers — keeps raw socket calls out of application code.

/**
 * @brief  Opens a UDP socket.
 * @param  bind_port  Port to bind for receiving; 0 = send-only socket.
 * @return File descriptor, or -1 on failure.
 */
int  udp_open(uint16_t bind_port);
void udp_close(int sock);
void udp_set_rcvbuf(int sock, int bytes);
void udp_set_send_timeout(int sock, int seconds);
void udp_set_recv_timeout(int sock, int seconds);

/**
 * @brief  Blocking receive.
 * @param  sock  Socket file descriptor.
 * @param  buf   Destination buffer.
 * @param  len   Size of buf in bytes.
 * @param  src   Filled with the sender's address, or NULL if not needed.
 * @return Number of bytes received, or -1 on error.
 */
int  udp_rx(int sock, void *buf, size_t len, struct sockaddr_in *src);

/**
 * @brief  Non-blocking receive.
 * @param  sock  Socket file descriptor.
 * @param  buf   Destination buffer.
 * @param  len   Size of buf in bytes.
 * @return Number of bytes received, 0 if nothing is waiting, or -1 on error.
 */
int  udp_try_recv(int sock, void *buf, size_t len);

void udp_tx(int sock, const struct sockaddr_in *dst, const void *buf, size_t len);
struct sockaddr_in udp_addr(const char *ip, uint16_t port);

/**
 * @brief  Returns a static string with the dotted-decimal IP of addr.
 * @param  addr  Address to format.
 * @return Pointer to a static buffer — not reentrant, overwritten on the next call.
 */
const char *udp_ip_str(const struct sockaddr_in *addr);
