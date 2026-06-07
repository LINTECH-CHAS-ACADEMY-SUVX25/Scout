#pragma once
#include <stdbool.h>

// Initialises LVGL, registers the display and touch drivers, starts the tick
// task, and builds the initial UI layout.
void lvgl_port_init(void);

// Shows the animated intro overlay; it removes itself when the animation ends.
void lvgl_port_intro_screen(void);

// Updates connection indicator. Call from ui_tick() when state changes.
void lvgl_port_ui_update(bool connected);

// Runs one LVGL render pass. Call once per render loop tick.
void lvgl_port_render_frame(void);
