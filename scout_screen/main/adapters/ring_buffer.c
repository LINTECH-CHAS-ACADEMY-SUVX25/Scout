#include "ring_buffer.h"

void ring_push(ring_buf_t *r, int32_t val)
{
    r->buf[r->head] = val;
    r->head = (r->head + 1) & (RING_CAPACITY - 1);
    if(r->count < RING_CAPACITY) r->count++;
}

int32_t ring_last(const ring_buf_t *r)
{
    if(!r->count) return 0;
    return r->buf[(r->head + RING_CAPACITY - 1) & (RING_CAPACITY - 1)];
}

int32_t ring_avg(const ring_buf_t *r)
{
    if(!r->count) return 0;
    int64_t sum = 0;
    for(uint32_t i = 0; i < RING_CAPACITY; i++) sum += r->buf[i];
    return (int32_t)(sum / (int64_t)r->count);
}

int32_t ring_min(const ring_buf_t *r)
{
    if(!r->count) return 0;
    int32_t min = ring_last(r);
    for(uint32_t i = 1; i < r->count; i++) {
        int32_t v = r->buf[(r->head + RING_CAPACITY - 1 - i) & (RING_CAPACITY - 1)];
        if(v < min) min = v;
    }
    return min;
}

int32_t ring_max(const ring_buf_t *r)
{
    if(!r->count) return 0;
    int32_t max = ring_last(r);
    for(uint32_t i = 1; i < r->count; i++) {
        int32_t v = r->buf[(r->head + RING_CAPACITY - 1 - i) & (RING_CAPACITY - 1)];
        if(v > max) max = v;
    }
    return max;
}

uint32_t ring_fps_tenths(const ring_buf_t *r)
{
    if(!r->count) return 0;
    int64_t sum = 0;
    for(uint32_t i = 0; i < RING_CAPACITY; i++) sum += r->buf[i];
    if(!sum) return 0;
    return (uint32_t)(10000ULL * r->count / (uint64_t)sum);
}

stat_snap_t ring_snap(const ring_buf_t *r)
{
    return (stat_snap_t){ ring_last(r), ring_avg(r) };
}
