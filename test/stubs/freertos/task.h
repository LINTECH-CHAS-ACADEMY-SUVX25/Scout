#pragma once
#include <stdint.h>
typedef void *TaskHandle_t;
#define pdMS_TO_TICKS(ms) (ms)
static inline int  xTaskCreate(void (*f)(void *), const char *n, int s, void *p, int pri, TaskHandle_t *h)
    { (void)f;(void)n;(void)s;(void)p;(void)pri;(void)h; return 1; }
static inline void vTaskDelay(uint32_t ticks) { (void)ticks; }
