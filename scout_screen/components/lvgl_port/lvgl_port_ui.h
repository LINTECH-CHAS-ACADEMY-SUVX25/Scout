#pragma once
#include <stdint.h>
#include <stdbool.h>

// Builds the full UI layout on the active screen. LVGL must be initialised.
void    lvgl_port_ui_init(void);

// Shows the intro overlay with a loading bar that fills over total_steps
// calls to lvgl_port_intro_step(). Not thread safe — call both functions
// before the render task starts.
void    lvgl_port_intro_screen(uint8_t total_steps);

// Advances the loading bar one step, shows label as the current init step
// and renders it to the screen immediately. After the last step the overlay
// removes itself once the render loop is running.
void    lvgl_port_intro_step(const char *label);

// Updates the WiFi signal symbol: 0 none (red dot), 1 low, 2 mid, 3 full.
void    lvgl_port_ui_update(uint8_t wifi_level);

// Returns the current RC command byte derived from the joystick position.
uint8_t lvgl_port_get_cmd(void);
