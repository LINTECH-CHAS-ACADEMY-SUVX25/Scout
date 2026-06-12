#include "screen_state.h"
#include "esp_timer.h"

// Shared state hub for scout_screen tasks.
// Centralises everything multiple tasks need to read or write: connection status,
// performance ring buffers, and render loop timing.
// stream_run (core 0) writes screen_status; sets transfer and bytes slots each frame.
// render_run (core 1) drives timing via screen_state_tick/tick_split with its own
// screen_tick_t context so the two tasks never share mutable timing state.

screen_status_t screen_status;

static ring_buf_t s_render_loop;
static ring_buf_t s_stream_loop;
static ring_buf_t s_decode;
static ring_buf_t s_blit;
static ring_buf_t s_lvgl;
static ring_buf_t s_disp;
static ring_buf_t s_transfer;
static ring_buf_t s_rx_interval;
static ring_buf_t s_bytes;

static uint32_t          s_frame_count;
static volatile uint32_t s_last_rx_ms;

static cam_diag_pkt_t    s_cam_status;

void screen_state_render_tick_init(screen_tick_t *ctx)
{
    ctx->loop_ring              = &s_render_loop;
    ctx->lvgl.ring              = &s_lvgl;
    ctx->decode.ring            = &s_decode;
    ctx->blit.ring              = &s_blit;
    ctx->blit.counts_frame      = true;
    ctx->blit.interval_ring     = &s_disp;
}

void screen_state_stream_tick_init(screen_tick_t *ctx)
{
    ctx->loop_ring                    = &s_stream_loop;
    ctx->transfer.ring                = &s_transfer;
    ctx->transfer.interval_ring       = &s_rx_interval;
    ctx->transfer.updates_streaming   = true;
    ctx->bytes.ring                   = &s_bytes;
}

void screen_state_tick(screen_tick_t *ctx)
{
    int64_t now = esp_timer_get_time();

    if(ctx->tick_start_us) {
        if(ctx->loop_ring)
            ring_push(ctx->loop_ring, (int32_t)((now - ctx->tick_start_us) / 1000));

        tick_slot_t *slots[] = {&ctx->lvgl, &ctx->decode, &ctx->blit, &ctx->transfer, &ctx->bytes};
        for(size_t i = 0; i < sizeof(slots) / sizeof(*slots); i++) {
            tick_slot_t *s = slots[i];
            if(!s->done || !s->ring) continue;
            ring_push(s->ring, s->ms);
            if(s->counts_frame) s_frame_count++;
            if(s->interval_ring || s->updates_streaming) {
                uint32_t now_ms = (uint32_t)(now / 1000);
                if(s->interval_ring) {
                    if(s->prev_interval_ms)
                        ring_push(s->interval_ring, (int32_t)(now_ms - s->prev_interval_ms));
                    s->prev_interval_ms = now_ms;
                }
                if(s->updates_streaming) s_last_rx_ms = now_ms;
            }
        }
    }

    ctx->tick_start_us  = now;
    ctx->split_start_us = now;

    tick_slot_t *slots[] = {&ctx->lvgl, &ctx->decode, &ctx->blit, &ctx->transfer, &ctx->bytes};
    for(size_t i = 0; i < sizeof(slots) / sizeof(*slots); i++) {
        slots[i]->ms   = 0;
        slots[i]->done = false;
    }
}

void screen_state_tick_split(screen_tick_t *ctx, tick_slot_t *slot)
{
    int64_t now         = esp_timer_get_time();
    slot->ms            = (int32_t)((now - ctx->split_start_us) / 1000);
    ctx->split_start_us = now;
    slot->done          = true;
}

bool screen_state_is_streaming(void)
{
    return s_last_rx_ms &&
           (uint32_t)(esp_timer_get_time() / 1000) - s_last_rx_ms < 2000;
}

void screen_state_get(screen_state_t *out)
{
    out->frame_count     = s_frame_count;
    out->render_loop     = ring_snap(&s_render_loop);
    out->stream_loop     = ring_snap(&s_stream_loop);
    out->decode          = ring_snap(&s_decode);
    out->blit            = ring_snap(&s_blit);
    out->lvgl            = ring_snap(&s_lvgl);
    out->disp            = ring_snap(&s_disp);
    out->transfer        = ring_snap(&s_transfer);
    out->rx_interval     = ring_snap(&s_rx_interval);
    out->frame_bytes     = ring_snap(&s_bytes);
    out->rx_fps_tenths   = ring_fps_tenths(&s_rx_interval);
    out->disp_fps_tenths = ring_fps_tenths(&s_disp);
}

void screen_state_set_cam(const cam_diag_pkt_t *pkt)
{
    s_cam_status = *pkt;
}

void screen_state_get_cam(cam_diag_pkt_t *out)
{
    *out = s_cam_status;
}
