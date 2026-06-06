#pragma once
#include <stdint.h>
#include <stdbool.h>

void    ui_init(void);
void    ui_intro_screen(void);
void    ui_tick(void);
uint8_t ui_get_cmd(void);
void    ui_set_connected(bool connected);
void    ui_input_event(int dx, int dy);
void    ui_input_release(void);
void    ui_render_frame(void);
