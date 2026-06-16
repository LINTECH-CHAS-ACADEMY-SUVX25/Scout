#pragma once
#include <stdint.h>
#include <stdbool.h>

// Physical display dimensions in pixels.
#define SCREEN_W 1024
#define SCREEN_H 600

void  display_init(void);
void  display_refresh(void);
void *display_get_fb(int index);
void  display_draw_bitmap(int x1, int y1, int x2, int y2, const void *pixels);
bool  display_read_touch(uint16_t *x, uint16_t *y);

/** @brief Copies pixels into framebuffer 0 at (x, y) for a w × h region. */
void  display_blit_region(int x, int y, int w, int h, const void *pixels);

/** @brief Clears a w × h region at (x, y) to black in framebuffer 0. */
void  display_clear_region(int x, int y, int w, int h);
