#pragma once

// Allocates the camera canvas buffer in PSRAM.
void render_init(void);

// Passed directly to xTaskCreatePinnedToCore — do not call from application code.
void render_run(void *arg);
