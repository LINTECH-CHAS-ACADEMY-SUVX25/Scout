#pragma once
#include "lvgl_port_ui.h"

// Initialises LVGL, registers the display and touch drivers, starts the tick
// task, and builds the initial UI layout.
void lvgl_port_init(void);

// Runs one LVGL render pass. Call once per render loop tick.
void lvgl_port_render_frame(void);
