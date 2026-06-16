#pragma once
#include "ring_buffer.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t     ms;
    bool        done;
    bool        counts_frame;        // if set, increments frame_count when committed
    bool        updates_streaming;   // if set, notifies screen_state of a new rx timestamp
    ring_buf_t *ring;                // target ring; NULL = slot inactive
    ring_buf_t *interval_ring;       // if set, records time between consecutive done events
    uint32_t    prev_interval_ms;    // internal state — do not set manually
} tick_slot_t;

typedef struct {
    int64_t     tick_start_us;
    int64_t     split_start_us;
    ring_buf_t *loop_ring;           // target ring for loop time; NULL = don't record
    tick_slot_t lvgl;
    tick_slot_t decode;
    tick_slot_t blit;
    tick_slot_t transfer;
    tick_slot_t bytes;
} screen_tick_t;

// Performance metrics snapshot returned by screen_stats_get.
typedef struct {
    uint32_t    frame_count;
    stat_snap_t render_loop;
    stat_snap_t stream_loop;
    stat_snap_t decode;
    stat_snap_t blit;
    stat_snap_t lvgl;
    stat_snap_t disp;
    stat_snap_t transfer;
    stat_snap_t rx_interval;
    stat_snap_t frame_bytes;
    uint32_t    rx_fps_tenths;
    uint32_t    disp_fps_tenths;
} screen_stats_t;

/** @brief Wires ring pointers and flags into ctx for the render task. Call once before the loop starts. */
void screen_stats_render_tick_init(screen_tick_t *ctx);

/** @brief Wires ctx for the stream task. Call once before the loop starts. */
void screen_stats_stream_tick_init(screen_tick_t *ctx);

/**
 * @brief Call at the top of any task loop. Commits the previous iteration's splits into the
 *        ring buffers and starts a fresh timer.
 */
void screen_stats_tick(screen_tick_t *ctx);

/**
 * @brief Records elapsed time since the previous split (or tick) into slot.
 * @param slot Pointer to the named field: &ctx->lvgl, &ctx->decode, etc.
 */
void screen_stats_tick_split(screen_tick_t *ctx, tick_slot_t *slot);

void screen_stats_get(screen_stats_t *out);
