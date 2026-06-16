#pragma once
#include <stdbool.h>

void lvgl_port_init(void);

/**
 * @brief Registers the rectangle owned by the direct video blit.
 *        While the video lock is on, the LVGL flush never writes inside this rectangle.
 */
void lvgl_port_set_video_region(int x, int y, int w, int h);

/**
 * @brief Enables/disables protection of the video region.
 *        Set on while a live frame owns the camera area; set off so the UI may draw there otherwise.
 */
void lvgl_port_video_lock(bool on);

void lvgl_port_render_frame(void);
