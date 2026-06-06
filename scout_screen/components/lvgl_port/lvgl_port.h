#pragma once
#include <stdint.h>
#include <stdbool.h>

void  lvgl_port_init(void);
void  lvgl_port_intro_screen(void);
void  lvgl_port_ui_update(bool connected);
void  lvgl_port_render_frame(void);
