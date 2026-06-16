#pragma once
#include <stdint.h>

/** @brief Builds the joystick panel including the stick, halo and command badges. */
void joy_panel_build(void);

void scout_ui_get_joy(int16_t *x, int16_t *y);
