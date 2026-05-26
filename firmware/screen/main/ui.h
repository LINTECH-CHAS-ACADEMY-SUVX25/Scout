#pragma once
#include <stdint.h>
#include <stdbool.h>

void    ui_init(void);
uint8_t ui_get_cmd(void);
void    ui_set_connected(bool connected);
void    ui_set_fps(uint32_t fps);
void    ui_tick(void);
