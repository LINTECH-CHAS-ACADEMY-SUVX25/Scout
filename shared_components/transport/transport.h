#pragma once
#include <stdint.h>
#include <stddef.h>
#include <arpa/inet.h>

// Protocol-agnostic datagram transport — current implementation: UDP over WiFi.
// To swap protocols, replace transport.c; callers never include lwip headers.

// Opaque IPv4 endpoint. Value type: copyable, storable as a static variable.
typedef struct {
    uint32_t ip;   // network byte order
    uint16_t port; // network byte order
} transport_addr_t;

// Open a socket bound to bind_port. Use 0 for send-only.
// Returns a file-descriptor handle, or -1 on error.
int  transport_open(uint16_t bind_port);
// Close a handle returned by transport_open.
void transport_close(int handle);
// Set the socket receive buffer size in bytes.
void transport_set_rcvbuf(int handle, int bytes);
// Set the send timeout; blocks at most seconds before returning.
void transport_set_send_timeout(int handle, int seconds);

// Blocking receive. Fills *src with the sender's address (may be NULL).
// Returns bytes received, or -1 on error.
int  transport_recv(int handle, void *buf, size_t len, transport_addr_t *src);

// Non-blocking receive. Returns 0 if nothing is pending.
int  transport_try_recv(int handle, void *buf, size_t len);

// Send buf to dst. Fire-and-forget; no return value.
void transport_send(int handle, const transport_addr_t *dst,
                    const void *buf, size_t len);

// Build an address from a dotted-decimal IP string and a host-byte-order port.
transport_addr_t transport_make_addr(const char *ip, uint16_t port);

// Copy src and replace its port (host byte order).
transport_addr_t transport_addr_with_port(const transport_addr_t *src,
                                          uint16_t port);

// Return a static dotted-decimal string for the address IP.
const char *transport_addr_ip(const transport_addr_t *addr);
