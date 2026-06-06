#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Decode a JPEG image to RGB565 little-endian.
// outbuf must be 16-byte aligned and large enough for the decoded image.
// Returns true on success; out_width/out_height are optional.
bool jpeg_decode_rgb565(const uint8_t *inbuf, int inbuf_len,
                        uint8_t *outbuf, size_t outbuf_size,
                        uint16_t *out_width, uint16_t *out_height);
