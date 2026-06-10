#pragma once
#include <stdint.h>

#define RING_CAPACITY 128   // must be a power of 2

typedef struct {
    int32_t  buf[RING_CAPACITY];
    uint32_t head;
    uint32_t count;
} ring_buf_t;

typedef struct {
    int32_t last;
    int32_t avg;
} stat_snap_t;

void        ring_push(ring_buf_t *r, int32_t val);
int32_t     ring_last(const ring_buf_t *r);
int32_t     ring_avg(const ring_buf_t *r);
int32_t     ring_min(const ring_buf_t *r);
int32_t     ring_max(const ring_buf_t *r);
uint32_t    ring_fps_tenths(const ring_buf_t *r);
stat_snap_t ring_snap(const ring_buf_t *r);
