#pragma once

// Physical display dimensions in pixels.
#define SCREEN_W 1024
#define SCREEN_H 600

// Initialises the LCD panel, touch controller, and framebuffers.
void  display_init(void);

// Triggers a manual refresh of the RGB panel.
void  display_refresh(void);

// Returns the panel handle (esp_lcd_panel_handle_t).
void *display_get_panel(void);

// Returns the touch handle (esp_lcd_touch_handle_t).
void *display_get_touch(void);

// Returns a pointer to framebuffer 0 or 1.
void *display_get_fb(int index);
