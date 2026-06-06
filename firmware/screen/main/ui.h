#pragma once
#include <stdint.h>
#include <stdbool.h>

void    ui_init(void);
void    ui_intro_screen(void);
void    ui_tick(void);
uint8_t ui_get_cmd(void);
void    ui_set_connected(bool connected);
void    ui_set_fps(uint32_t fps);
void    ui_input_event(int dx, int dy);
void    ui_input_release(void);
void    ui_render_frame(void);
void    ui_canvas_init(uint8_t *buf, int w, int h);
void    ui_canvas_invalidate(void);
