#include "ui.h"
#include "lvgl_port.h"
#include "rc_protocol.h"
#include "esp_timer.h"
#include <stdlib.h>
#include <stdbool.h>

// ── State ─────────────────────────────────────────────────────────────────────

static uint8_t           s_cmd       = CMD_STOP;
static volatile bool     s_connected = false;
static volatile uint32_t s_fps_val   = 0;
static volatile bool     s_ui_dirty  = false;

static float   s_sim_temp  = 23.4f;
static float   s_sim_humi  = 45.0f;
static float   s_sim_pres  = 1013.2f;
static int64_t s_sim_timer = 0;

// ── Public API ────────────────────────────────────────────────────────────────

uint8_t ui_get_cmd(void) { return s_cmd; }

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

// ── Sensor simulation ─────────────────────────────────────────────────────────

static bool update_sensor_sim(void)
{
    int64_t now = esp_timer_get_time();
    if (now - s_sim_timer < 2000000) return false;
    s_sim_timer = now;

    s_sim_temp += (float)((rand() % 11) - 5) * 0.1f;
    s_sim_humi += (float)((rand() % 11) - 5) * 0.3f;
    s_sim_pres += (float)((rand() % 7)  - 3) * 0.1f;
    if (s_sim_humi < 0.0f)   s_sim_humi = 0.0f;
    if (s_sim_humi > 100.0f) s_sim_humi = 100.0f;
    return true;
}

// ── Tick / init ───────────────────────────────────────────────────────────────

void ui_tick(void)
{
    bool sensor_updated = update_sensor_sim();
    if (!s_ui_dirty && !sensor_updated) return;
    s_ui_dirty = false;
    lvgl_port_ui_update(s_connected, s_fps_val, s_sim_temp, s_sim_humi, s_sim_pres);
}

void ui_init(void)
{
    lvgl_port_init();
}

// ── Render / canvas (wraps lvgl_port so render.c stays lvgl-free) ─────────────

static void *s_canvas;

void ui_render_frame(void)
{
    lvgl_port_render_frame();
}

void ui_canvas_init(uint8_t *buf, int w, int h)
{
    s_canvas = lvgl_port_create_video_canvas(buf, w, h);
}

void ui_canvas_invalidate(void)
{
    lvgl_port_canvas_invalidate(s_canvas);
}
