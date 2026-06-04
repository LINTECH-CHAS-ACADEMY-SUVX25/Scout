#include "ui.h"
#include "lvgl_port.h"
#include "rc_protocol.h"
#include "esp_timer.h"
#include <stdbool.h>

// ── State ─────────────────────────────────────────────────────────────────────
/*
static float s_temp = 0;
static float s_humi = 0;
static float s_pres = 0;
*/
static uint8_t           s_cmd       = CMD_STOP;
static volatile bool     s_connected = false;
static volatile uint32_t s_fps_val   = 0;
static volatile bool     s_ui_dirty  = false;

// ── Public API ────────────────────────────────────────────────────────────────

uint8_t ui_get_cmd(void) { return s_cmd; }
/*
void ui_set_sensor_data(float temp, float humi, float pres)
{
    s_temp = temp;
    s_humi = humi;
    s_pres = pres;
}
*/
void ui_set_connected(bool connected)
{
    s_connected = connected;
    s_ui_dirty  = true;
}

void ui_set_fps(uint32_t fps)
{
    s_fps_val  = fps;
    s_ui_dirty = true;
}

void ui_input_event(int dx, int dy)
{
    uint8_t cmd = CMD_STOP;
    if (dy < -15) cmd |= CMD_FORWARD;
    if (dy >  15) cmd |= CMD_BACKWARD;
    if (dx < -15) cmd |= CMD_LEFT;
    if (dx >  15) cmd |= CMD_RIGHT;
    s_cmd = cmd;
}

void ui_input_release(void)
{
    s_cmd = CMD_STOP;
}

// ── Tick / init ───────────────────────────────────────────────────────────────

void ui_tick(void)
{
    if (!s_ui_dirty) return;
    s_ui_dirty = false;
    lvgl_port_ui_update(s_connected, s_fps_val); 
    /*
    change to -> lvgl_port_ui_update(s_temp, s_humi, s_pres, s_connected, s_fps_val); when cam sends sensor values (or mock data)
    also change the function parameters in lvgl_port
    */
}

void ui_init(void)
{
    lvgl_port_init();
}

// ── Render / canvas (wraps lvgl_port so render.c stays lvgl-free) ─────────────

static void *s_canvas;

void ui_render_frame(void)
{
    lvgl_port_render_frame(); // wrapper function to abstract lvgl from ui.c
}

void ui_canvas_init(uint8_t *buf, int w, int h)
{
    s_canvas = lvgl_port_create_video_canvas(buf, w, h);
}

void ui_canvas_invalidate(void)
{
    lvgl_port_canvas_invalidate(s_canvas);
}
