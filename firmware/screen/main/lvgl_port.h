#pragma once
#include <stdint.h>
#include <stdbool.h>

void  lvgl_port_init(void);
void  lvgl_port_ui_update(bool connected, uint32_t fps,
                           float temp, float humi, float pres);
void *lvgl_port_create_video_canvas(uint8_t *buf, int w, int h);
void  lvgl_port_canvas_invalidate(void *canvas);
void  lvgl_port_render_frame(void);
