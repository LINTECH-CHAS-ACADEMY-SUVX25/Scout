#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** @brief Allocates a 16-byte-aligned PSRAM buffer sized for w * h RGB565 pixels. */
void      jpeg_init_canvas(uint16_t w, uint16_t h);

uint16_t *jpeg_canvas_get(void);

/**
 * @brief Decodes a JPEG image to RGB565 little-endian.
 *        outbuf must be 16-byte aligned and large enough for width * height * 2 bytes.
 * @return true on success; out_width and out_height are optional.
 */
bool jpeg_decode_rgb565(const uint8_t *inbuf, int inbuf_len,
                        uint8_t *outbuf, size_t outbuf_size,
                        uint16_t *out_width, uint16_t *out_height);

/**
 * @brief Encodes an RGB565 little-endian image to JPEG.
 * @param quality 1 (smallest) – 100 (best); 20–40 is reasonable for streaming.
 */
bool jpeg_encode_rgb565(const uint8_t *inbuf, uint16_t width, uint16_t height,
                        uint8_t *outbuf, size_t outbuf_size, size_t *out_len,
                        int quality);
