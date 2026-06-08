#pragma once
#include "IDisplay.hpp"

// Allocates the camera canvas buffer and starts the render task on core 1.
// The render task drives LVGL, blits decoded frames, and sends RC commands.
// render never includes display.h — it draws through IDisplay.
void render_init(IDisplay *display);
