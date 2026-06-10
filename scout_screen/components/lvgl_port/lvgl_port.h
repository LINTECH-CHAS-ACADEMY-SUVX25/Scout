#pragma once

// Initialises LVGL, registers the display and touch drivers and starts the
// tick task.
void lvgl_port_init(void);

// Runs one LVGL render pass. Call once per render loop tick.
void lvgl_port_render_frame(void);
