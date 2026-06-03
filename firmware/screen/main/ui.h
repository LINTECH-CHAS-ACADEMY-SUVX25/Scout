#pragma once
#include <stdint.h>
#include <stdbool.h>

void    ui_init(void);
void    ui_tick(void);
uint8_t ui_get_cmd(void);
void    ui_set_connected(bool connected);
void    ui_set_fps(uint32_t fps);
void    ui_input_event(int dx, int dy);
void    ui_input_release(void);
