#pragma once

// Allocates the camera canvas buffer and starts the render task on core 1.
// The render task drives LVGL, blits decoded frames, and sends RC commands.
void render_init(void);
