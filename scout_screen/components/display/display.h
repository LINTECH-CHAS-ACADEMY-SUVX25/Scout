#pragma once
#include <stdint.h>
#include <stdbool.h>

// Physical display dimensions in pixels.
#define SCREEN_W 1024
#define SCREEN_H 600

// Initialises the LCD panel, touch controller, and framebuffers.
void  display_init(void);

// Triggers a manual refresh of the RGB panel.
void  display_refresh(void);

// Returns a pointer to framebuffer 0 or 1.
void *display_get_fb(int index);

// Blits a pixel rectangle to the LCD panel.
void  display_draw_bitmap(int x1, int y1, int x2, int y2, const void *pixels);

// Reads the current touch state. Returns true if the panel is being touched.
bool  display_read_touch(uint16_t *x, uint16_t *y);
