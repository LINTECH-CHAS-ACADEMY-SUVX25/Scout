#pragma once
#include <stdint.h>
#include <stdbool.h>

// Builds the full UI layout on the active screen. LVGL must be initialised.
void    lvgl_port_ui_init(void);

// Shows the animated intro overlay; it removes itself when the animation ends.
void    lvgl_port_intro_screen(void);

// Updates the connection status indicator. Call when connected state changes.
void    lvgl_port_ui_update(bool connected);

// Returns the current RC command byte derived from the joystick position.
uint8_t lvgl_port_get_cmd(void);
