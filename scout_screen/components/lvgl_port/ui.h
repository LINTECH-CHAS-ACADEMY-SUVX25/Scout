#pragma once
#include <stdint.h>
#include <stdbool.h>

// Initialises LVGL and the UI layout.
void    ui_init(void);

// Processes pending UI state changes. Call once per render loop tick.
void    ui_tick(void);

// Renders the current LVGL frame to the display.
void    ui_render_frame(void);

// Returns the current RC command byte derived from the joystick position.
uint8_t ui_get_cmd(void);

// Updates the connection status indicator.
void    ui_set_connected(bool connected);

// Updates the joystick command from a touch delta (pixels from centre).
void    ui_input_event(int dx, int dy);

// Releases the joystick (resets command to CMD_STOP).
void    ui_input_release(void);
