#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Allocates PSRAM ping-pong buffers and packet receive buffer; creates the frame mutex.
void     frame_buf_init(void);

// Returns a pointer to the assembly buffer for stream_run to write fragments into.
uint8_t *frame_buf_asm(void);

// Returns the packet receive buffer used by udp_rx inside stream_run.
uint8_t *frame_buf_pkt(void);

// Swaps the assembled frame in for decoding when the decoder is free; drops it otherwise.
// now_ms is used to track liveness; the caller is responsible for pushing transfer stats.
void     frame_buf_publish(uint32_t now_ms, uint32_t len);

// Acquires the latest published frame for decoding. Sets *src and *len, marks the buffer
// as in-use so publish cannot overwrite it. Returns false if no new frame is ready.
// Must be followed by exactly one frame_buf_release() call.
bool     frame_buf_try_acquire(const uint8_t **src, uint32_t *len);

// Releases the decoding lock so the next published frame can be swapped in.
void     frame_buf_release(void);

// Returns true if a full frame arrived within the last 2 seconds.
bool     frame_buf_is_streaming(void);
