#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint32_t frame_count;
    uint32_t last_frame_bytes;
    int32_t  last_transfer_ms;
    int32_t  last_decode_ms;
    uint32_t avg_frame_bytes;
    int32_t  avg_transfer_ms;
    int32_t  avg_decode_ms;
    int32_t  last_blit_ms;
    int32_t  avg_blit_ms;
    int32_t  last_lvgl_ms;
    int32_t  avg_lvgl_ms;
    uint32_t rx_fps_tenths;    // rate at which complete frames arrive from camera
    uint32_t disp_fps_tenths;  // rate at which frames are actually decoded and displayed
} stream_stats_t;

// Allocates PSRAM ping-pong buffers and packet receive buffer; creates the frame mutex.
void     frame_buf_init(void);

// Returns a pointer to the assembly buffer for stream_run to write fragments into.
uint8_t *frame_buf_asm(void);

// Returns the packet receive buffer used by udp_rx inside stream_run.
uint8_t *frame_buf_pkt(void);

// Swaps the assembled frame in for decoding when the decoder is free; drops it otherwise.
void     frame_buf_publish(uint32_t len, int32_t transfer_ms);

// Decodes the most recently published frame into out_buf as RGB565. Call from render_run only.
bool     frame_buf_try_decode(uint8_t *out_buf, size_t out_size);

// Returns true if a full frame arrived within the last 2 seconds.
bool     frame_buf_is_connected(void);

// Records that a frame was fully rendered (decode + blit complete); updates disp_fps_tenths.
void     frame_buf_record_disp_frame(void);

// Records blit and LVGL render durations from render_run.
void     frame_buf_record_blit(int32_t ms);
void     frame_buf_record_lvgl(int32_t ms);

// Copies the latest stream statistics into out.
void     frame_buf_get_stats(stream_stats_t *out);
