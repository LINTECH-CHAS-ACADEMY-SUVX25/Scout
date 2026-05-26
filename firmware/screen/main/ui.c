#include "ui.h"
#include "rc_protocol.h"
#include "lvgl.h"
#include "esp_timer.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// ─── State ────────────────────────────────────────────────────────────────────

static uint8_t   s_cmd       = CMD_STOP;
static lv_obj_t *s_knob;
static lv_obj_t *s_halo;

static lv_obj_t *s_conn_dot;
static lv_obj_t *s_conn_label;
static lv_obj_t *s_fps_label;
static lv_obj_t *s_cmd_badges[5]; // FWD BWD STOP LEFT RIGHT
static lv_obj_t *s_bar_motor_l;
static lv_obj_t *s_bar_motor_r;
static lv_obj_t *s_bar_battery;
static lv_obj_t *s_val_speed;
static lv_obj_t *s_val_battery;
static lv_obj_t *s_val_rssi;
static lv_obj_t *s_val_rtt;
static lv_obj_t *s_val_temp;
static lv_obj_t *s_val_humi;
static lv_obj_t *s_val_pres;
static lv_obj_t *s_val_uptime;
static lv_obj_t *s_val_watchdog;
static lv_obj_t *s_val_cmd_hex;

static volatile bool     s_connected = false;
static volatile uint32_t s_fps_val   = 0;
static volatile bool     s_ui_dirty  = false;

static float   s_sim_temp  = 23.4f;
static float   s_sim_humi  = 45.0f;
static float   s_sim_pres  = 1013.2f;
static int64_t s_sim_timer = 0;

// ─── Public API ───────────────────────────────────────────────────────────────

uint8_t ui_get_cmd(void)
{
    return s_cmd;
}

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

// ─── Tick / update ────────────────────────────────────────────────────────────

static void update_sensor_sim(void)
{
    int64_t now = esp_timer_get_time();
    if (now - s_sim_timer < 2000000) return;
    s_sim_timer = now;

    s_sim_temp += (float)((rand() % 11) - 5) * 0.1f;
    s_sim_humi += (float)((rand() % 11) - 5) * 0.3f;
    s_sim_pres += (float)((rand() % 7)  - 3) * 0.1f;
    if (s_sim_humi < 0.0f)   s_sim_humi = 0.0f;
    if (s_sim_humi > 100.0f) s_sim_humi = 100.0f;

    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f °C", s_sim_temp);
    lv_label_set_text(s_val_temp, buf);
    snprintf(buf, sizeof(buf), "%.0f %%", s_sim_humi);
    lv_label_set_text(s_val_humi, buf);
    snprintf(buf, sizeof(buf), "%.1f hPa", s_sim_pres);
    lv_label_set_text(s_val_pres, buf);
}

static void update_cmd_badges(uint8_t cmd)
{
    // FWD BWD STOP LEFT RIGHT
    static const uint8_t masks[5] = {
        CMD_FORWARD, CMD_BACKWARD, 0xFF, CMD_LEFT, CMD_RIGHT
    };
    for (int i = 0; i < 5; i++) {
        bool active = (i == 2)
            ? (cmd == CMD_STOP)
            : (cmd & masks[i]);
        lv_obj_set_style_bg_color(s_cmd_badges[i],
            active ? lv_color_hex(0xE8F4FF) : lv_color_hex(0xEEEEEA), 0);
        lv_obj_set_style_border_color(s_cmd_badges[i],
            active ? lv_color_hex(0xB5D4F4) : lv_color_hex(0xDDDDD8), 0);

        lv_obj_t *lbl = lv_obj_get_child(s_cmd_badges[i], 0);
        if (lbl) {
            lv_obj_set_style_text_color(lbl,
                active ? lv_color_hex(0x185FA5) : lv_color_hex(0xAAAAAA), 0);
        }
    }
}

void ui_tick(void)
{
    update_sensor_sim();

    if (!s_ui_dirty) return;
    s_ui_dirty = false;

    lv_obj_set_style_bg_color(s_conn_dot,
        s_connected ? lv_color_hex(0x4CAF50) : lv_color_hex(0xE24B4A), 0);
    lv_label_set_text(s_conn_label, s_connected ? "ansluten" : "väntar...");

    char buf[16];
    if (s_fps_val > 0)
        snprintf(buf, sizeof(buf), "%u fps", (unsigned)s_fps_val);
    else
        snprintf(buf, sizeof(buf), "-- fps");
    lv_label_set_text(s_fps_label, buf);
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

static lv_obj_t *make_obj(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                             lv_color_t color, const lv_font_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
    return l;
}

// Thin horizontal bar used in right panel
static lv_obj_t *make_bar(lv_obj_t *parent, int32_t w)
{
    lv_obj_t *track = make_obj(parent);
    lv_obj_set_size(track, w, 3);
    lv_obj_set_style_bg_color(track, lv_color_hex(0xE0E0DA), 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(track, 2, 0);

    lv_obj_t *fill = make_obj(track);
    lv_obj_set_size(fill, 0, 3);
    lv_obj_align(fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(fill, lv_color_hex(0x185FA5), 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(fill, 2, 0);
    return fill; // caller keeps the fill handle to update width
}

static lv_obj_t *create_joy_tick(lv_obj_t *parent, lv_align_t align,
                                  int32_t xo, int32_t yo, bool horiz)
{
    lv_obj_t *t = lv_obj_create(parent);
    lv_obj_set_size(t, horiz ? 7 : 1, horiz ? 1 : 7);
    lv_obj_align(t, align, xo, yo);
    lv_obj_set_style_bg_color(t, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(t, 1, 0);
    lv_obj_set_style_border_width(t, 0, 0);
    lv_obj_clear_flag(t, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return t;
}

// One row in the telemetry list: key on left, value label on right
static lv_obj_t *make_tele_row(lv_obj_t *parent, const char *key,
                                const char *init_val, lv_color_t val_color,
                                int32_t y)
{
    lv_obj_t *row = make_obj(parent);
    lv_obj_set_size(row, 172, 22);
    lv_obj_set_pos(row, 0, y);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);

    lv_obj_t *k = make_label(row, key,
        lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *v = make_label(row, init_val,
        val_color, &lv_font_montserrat_14);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, 0, 0);
    return v;
}

// ─── Joystick event ───────────────────────────────────────────────────────────

static void joy_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t       *base = lv_event_get_target(e);

    if (code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t  pt;
        lv_indev_get_point(indev, &pt);

        lv_area_t coords;
        lv_obj_get_coords(base, &coords);
        int dx = pt.x - (coords.x1 + coords.x2) / 2;
        int dy = pt.y - (coords.y1 + coords.y2) / 2;

        float mag = sqrtf((float)(dx * dx + dy * dy));
        if (mag > 52.0f) {
            dx = (int)(dx * 52.0f / mag);
            dy = (int)(dy * 52.0f / mag);
        }

        lv_obj_align(s_knob, LV_ALIGN_CENTER, dx, dy);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, dx, dy);

        lv_obj_set_style_transform_zoom(s_knob, 210, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x222222), 0);

        uint8_t cmd = CMD_STOP;
        if (dy < -15) cmd |= CMD_FORWARD;
        if (dy >  15) cmd |= CMD_BACKWARD;
        if (dx < -15) cmd |= CMD_LEFT;
        if (dx >  15) cmd |= CMD_RIGHT;
        s_cmd = cmd;
        update_cmd_badges(s_cmd);

        char buf[8];
        snprintf(buf, sizeof(buf), "0x%02X", s_cmd);
        lv_label_set_text(s_val_cmd_hex, buf);

    } else {
        lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_transform_zoom(s_knob, 256, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A3A3A), 0);
        s_cmd = CMD_STOP;
        update_cmd_badges(CMD_STOP);
        lv_label_set_text(s_val_cmd_hex, "0x00");
    }
}

// ─── ui_init ──────────────────────────────────────────────────────────────────

void ui_init(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // ── Topbar ──────────────────────────────────────────────────────────────
    lv_obj_t *topbar = make_obj(lv_scr_act());
    lv_obj_set_size(topbar, 1024, 36);
    lv_obj_align(topbar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(topbar, lv_color_hex(0x111417), 0);
    lv_obj_set_style_bg_opa(topbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(topbar, 0, 0);
    lv_obj_set_style_border_side(topbar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(topbar, lv_color_hex(0x1C1F24), 0);

    lv_obj_t *logo = make_label(topbar, "SCOUT",
        lv_color_hex(0x39FF14), &lv_font_montserrat_14);
    lv_obj_set_style_text_letter_space(logo, 6, 0);
    lv_obj_align(logo, LV_ALIGN_LEFT_MID, 14, 0);

    lv_obj_t *sub = make_label(topbar, "RC-TELEMETRI",
        lv_color_hex(0x333333), &lv_font_montserrat_14);
    lv_obj_set_style_text_letter_space(sub, 2, 0);
    lv_obj_align(sub, LV_ALIGN_LEFT_MID, 120, 0);

    // Connection dot + label
    s_conn_label = make_label(topbar, "väntar...",
        lv_color_hex(0x888888), &lv_font_montserrat_14);
    lv_obj_align(s_conn_label, LV_ALIGN_RIGHT_MID, -90, 0);

    s_conn_dot = make_obj(topbar);
    lv_obj_set_size(s_conn_dot, 6, 6);
    lv_obj_align(s_conn_dot, LV_ALIGN_RIGHT_MID, -170, 0);
    lv_obj_set_style_radius(s_conn_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_conn_dot, lv_color_hex(0xE24B4A), 0);
    lv_obj_set_style_bg_opa(s_conn_dot, LV_OPA_COVER, 0);

    s_fps_label = make_label(topbar, "-- fps",
        lv_color_hex(0x444444), &lv_font_montserrat_14);
    lv_obj_align(s_fps_label, LV_ALIGN_RIGHT_MID, -14, 0);

    // ── Bottombar ────────────────────────────────────────────────────────────
    lv_obj_t *botbar = make_obj(lv_scr_act());
    lv_obj_set_size(botbar, 1024, 32);
    lv_obj_align(botbar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(botbar, lv_color_hex(0x111417), 0);
    lv_obj_set_style_bg_opa(botbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(botbar, 0, 0);
    lv_obj_set_style_border_side(botbar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(botbar, lv_color_hex(0x1C1F24), 0);

    const char *bot_keys[] = { "IP", "UDP" };
    const char *bot_vals[] = { "192.168.1.42", "4210" };
    const int32_t key_w[]  = { 24, 38 };
    int32_t     bot_x      = 14;
    for (int i = 0; i < 2; i++) {
        lv_obj_t *k = make_label(botbar, bot_keys[i],
            lv_color_hex(0x444444), &lv_font_montserrat_14);
        lv_obj_align(k, LV_ALIGN_LEFT_MID, bot_x, 0);
        bot_x += key_w[i];
        lv_obj_t *v = make_label(botbar, bot_vals[i],
            lv_color_hex(0x667788), &lv_font_montserrat_14);
        lv_obj_align(v, LV_ALIGN_LEFT_MID, bot_x, 0);
        bot_x += (i == 0) ? 110 : 50;
    }
    lv_obj_t *rtos_k = make_label(botbar, "RTOS",
        lv_color_hex(0x444444), &lv_font_montserrat_14);
    lv_obj_align(rtos_k, LV_ALIGN_RIGHT_MID, -120, 0);
    lv_obj_t *rtos_v = make_label(botbar, "FREERTOS",
        lv_color_hex(0x667788), &lv_font_montserrat_14);
    lv_obj_align(rtos_v, LV_ALIGN_RIGHT_MID, -14, 0);

    // Content area between topbar and bottombar
    // y=36, height=600-36-32=532
    const int32_t CONTENT_Y = 36;
    const int32_t CONTENT_H = 600 - 36 - 32; // 532
    const int32_t LEFT_W    = 200;
    const int32_t RIGHT_W   = 160;
    const int32_t VIDEO_W   = 1024 - LEFT_W - RIGHT_W; // 664

    // ── Left panel ───────────────────────────────────────────────────────────
    lv_obj_t *left = make_obj(lv_scr_act());
    lv_obj_set_size(left, LEFT_W, CONTENT_H);
    lv_obj_set_pos(left, 0, CONTENT_Y);
    lv_obj_set_style_bg_color(left, lv_color_hex(0xF7F7F5), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(left, 1, 0);
    lv_obj_set_style_border_side(left, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(left, lv_color_hex(0xE8E8E4), 0);
    lv_obj_set_style_radius(left, 0, 0);

    // Telemetry section label
    lv_obj_t *tele_hdr = make_label(left, "TELEMETRI",
        lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
    lv_obj_set_style_text_letter_space(tele_hdr, 2, 0);
    lv_obj_align(tele_hdr, LV_ALIGN_TOP_LEFT, 14, 10);

    // Telemetry rows — y offsets from top of left panel
    lv_obj_t *tele_cont = make_obj(left);
    lv_obj_set_size(tele_cont, 172, 220);
    lv_obj_set_pos(tele_cont, 14, 28);

    s_val_speed    = make_tele_row(tele_cont, "hastighet", "0.0 m/s",  lv_color_hex(0x222222),   0);
    s_val_battery  = make_tele_row(tele_cont, "batteri",   "74 %",     lv_color_hex(0x854F0B),  22);
    s_val_rssi     = make_tele_row(tele_cont, "wifi rssi", "-58 dBm",  lv_color_hex(0x222222),  44);
    s_val_rtt      = make_tele_row(tele_cont, "rtt",       "12 ms",    lv_color_hex(0x2E7D32),  66);
    s_val_temp     = make_tele_row(tele_cont, "temp",      "23.4 °C",  lv_color_hex(0x222222),  88);
    s_val_humi     = make_tele_row(tele_cont, "fukt",      "45 %",     lv_color_hex(0x222222), 110);
    s_val_pres     = make_tele_row(tele_cont, "tryck",     "1013 hPa", lv_color_hex(0x222222), 132);
    s_val_uptime   = make_tele_row(tele_cont, "uptime",    "00:00:00", lv_color_hex(0x222222), 154);
    s_val_watchdog = make_tele_row(tele_cont, "watchdog",  "OK",       lv_color_hex(0x2E7D32), 176);
    s_val_cmd_hex  = make_tele_row(tele_cont, "cmd",       "0x00",     lv_color_hex(0x999999), 198);

    // Divider between telemetry and joystick
    lv_obj_t *divider = make_obj(left);
    lv_obj_set_size(divider, LEFT_W, 1);
    lv_obj_set_pos(divider, 0, CONTENT_H - 210);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0xE8E8E4), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

    // Joystick section label
    lv_obj_t *joy_hdr = make_label(left, "JOYSTICK",
        lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
    lv_obj_set_style_text_letter_space(joy_hdr, 2, 0);
    lv_obj_set_pos(joy_hdr, 14, CONTENT_H - 200);

    // Joystick base
    lv_obj_t *base = lv_obj_create(left);
    lv_obj_set_size(base, 130, 130);
    lv_obj_set_pos(base, (LEFT_W - 130) / 2, CONTENT_H - 180);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(base, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(base, 2, 0);
    lv_obj_set_style_border_color(base, lv_color_hex(0x2E2E2E), 0);
    lv_obj_set_style_border_opa(base, LV_OPA_COVER, 0);
    // NOTE: shadow removed — blurred shadow rendering into PSRAM framebuffers
    // was stalling the LVGL render pass and tripping the task watchdog.
    lv_obj_clear_flag(base, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESSING,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_RELEASED,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESS_LOST, NULL);

    // Guide ring
    lv_obj_t *guide = lv_obj_create(base);
    lv_obj_set_size(guide, 82, 82);
    lv_obj_align(guide, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(guide, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(guide, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(guide, 1, 0);
    lv_obj_set_style_border_color(guide, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_border_opa(guide, LV_OPA_COVER, 0);
    lv_obj_clear_flag(guide, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // Cardinal ticks
    create_joy_tick(base, LV_ALIGN_TOP_MID,    0,  7, false);
    create_joy_tick(base, LV_ALIGN_BOTTOM_MID, 0, -7, false);
    create_joy_tick(base, LV_ALIGN_LEFT_MID,   7,  0, true);
    create_joy_tick(base, LV_ALIGN_RIGHT_MID, -7,  0, true);

    // Halo
    s_halo = lv_obj_create(base);
    lv_obj_set_size(s_halo, 62, 62);
    lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_halo, lv_color_hex(0x888888), 0);
    lv_obj_set_style_bg_opa(s_halo, LV_OPA_20, 0);
    lv_obj_set_style_border_width(s_halo, 0, 0);
    lv_obj_clear_flag(s_halo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // Knob
    s_knob = lv_obj_create(base);
    lv_obj_set_size(s_knob, 46, 46);
    lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_bg_opa(s_knob, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_knob, 0, 0);
    // NOTE: shadow removed — same watchdog/stall reason as the base above.
    lv_obj_set_style_transform_pivot_x(s_knob, 23, 0);
    lv_obj_set_style_transform_pivot_y(s_knob, 23, 0);
    lv_obj_set_style_transform_zoom(s_knob, 256, 0);
    lv_obj_clear_flag(s_knob, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // Crosshair ring
    lv_obj_t *ch_ring = lv_obj_create(s_knob);
    lv_obj_set_size(ch_ring, 16, 16);
    lv_obj_align(ch_ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ch_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ch_ring, 1, 0);
    lv_obj_set_style_border_color(ch_ring, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_opa(ch_ring, LV_OPA_40, 0);
    lv_obj_clear_flag(ch_ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // Crosshair center dot
    lv_obj_t *ch_dot = lv_obj_create(s_knob);
    lv_obj_set_size(ch_dot, 4, 4);
    lv_obj_align(ch_dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ch_dot, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_opa(ch_dot, LV_OPA_60, 0);
    lv_obj_set_style_border_width(ch_dot, 0, 0);
    lv_obj_clear_flag(ch_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // CMD badges below joystick
    const char *badge_labels[5] = { "FWD", "BWD", "STP", "LFT", "RGT" };
    int32_t badge_x = 6;
    for (int i = 0; i < 5; i++) {
        lv_obj_t *badge = lv_obj_create(left);
        lv_obj_set_size(badge, 32, 18);
        lv_obj_set_pos(badge, badge_x, CONTENT_H - 38);
        lv_obj_set_style_radius(badge, 3, 0);
        lv_obj_set_style_bg_color(badge, lv_color_hex(0xEEEEEA), 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(badge, 1, 0);
        lv_obj_set_style_border_color(badge, lv_color_hex(0xDDDDD8), 0);
        lv_obj_set_style_pad_all(badge, 0, 0);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *bl = make_label(badge, badge_labels[i],
            lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
        lv_obj_align(bl, LV_ALIGN_CENTER, 0, 0);

        s_cmd_badges[i] = badge;
        badge_x += 35;
    }
    update_cmd_badges(CMD_STOP);

    // ── Video area ───────────────────────────────────────────────────────────
    lv_obj_t *video = make_obj(lv_scr_act());
    lv_obj_set_size(video, VIDEO_W, CONTENT_H);
    lv_obj_set_pos(video, LEFT_W, CONTENT_Y);
    lv_obj_set_style_bg_color(video, lv_color_hex(0xF0F0EC), 0);
    lv_obj_set_style_bg_opa(video, LV_OPA_COVER, 0);

    // Corner labels
    lv_obj_t *c_tl = make_label(video, "CAM // ESP32-CAM",
        lv_color_hex(0xCCCCCC), &lv_font_montserrat_14);
    lv_obj_align(c_tl, LV_ALIGN_TOP_LEFT, 10, 8);

    lv_obj_t *c_tr = make_label(video, "MJPEG",
        lv_color_hex(0xCCCCCC), &lv_font_montserrat_14);
    lv_obj_align(c_tr, LV_ALIGN_TOP_RIGHT, -10, 8);

    lv_obj_t *c_bl = make_label(video, "1024x600",
        lv_color_hex(0xCCCCCC), &lv_font_montserrat_14);
    lv_obj_align(c_bl, LV_ALIGN_BOTTOM_LEFT, 10, -8);

    lv_obj_t *c_br = make_label(video, "NO SIGNAL",
        lv_color_hex(0xCCCCCC), &lv_font_montserrat_14);
    lv_obj_align(c_br, LV_ALIGN_BOTTOM_RIGHT, -10, -8);

    // Center crosshair
    lv_obj_t *v_h = make_obj(video);
    lv_obj_set_size(v_h, 28, 1);
    lv_obj_align(v_h, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(v_h, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_opa(v_h, LV_OPA_COVER, 0);

    lv_obj_t *v_v = make_obj(video);
    lv_obj_set_size(v_v, 1, 28);
    lv_obj_align(v_v, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(v_v, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_opa(v_v, LV_OPA_COVER, 0);

    lv_obj_t *v_dot = make_obj(video);
    lv_obj_set_size(v_dot, 4, 4);
    lv_obj_align(v_dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(v_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(v_dot, lv_color_hex(0xBBBBBB), 0);
    lv_obj_set_style_bg_opa(v_dot, LV_OPA_COVER, 0);

    // ── Right panel ──────────────────────────────────────────────────────────
    lv_obj_t *right = make_obj(lv_scr_act());
    lv_obj_set_size(right, RIGHT_W, CONTENT_H);
    lv_obj_set_pos(right, LEFT_W + VIDEO_W, CONTENT_Y);
    lv_obj_set_style_bg_color(right, lv_color_hex(0xF7F7F5), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(right, 1, 0);
    lv_obj_set_style_border_side(right, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(right, lv_color_hex(0xE8E8E4), 0);
    lv_obj_set_style_radius(right, 0, 0);

    lv_obj_t *mot_hdr = make_label(right, "MOTORER",
        lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
    lv_obj_set_style_text_letter_space(mot_hdr, 2, 0);
    lv_obj_align(mot_hdr, LV_ALIGN_TOP_LEFT, 12, 10);

    // Motor bar rows
    const char *bar_keys[] = { "Motor L", "Motor R", "Batteri" };
    lv_obj_t  **bar_ptrs[] = { &s_bar_motor_l, &s_bar_motor_r, &s_bar_battery };
    int32_t     bar_y      = 32;

    for (int i = 0; i < 3; i++) {
        lv_obj_t *k = make_label(right, bar_keys[i],
            lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
        lv_obj_set_pos(k, 12, bar_y);

        lv_obj_t *pct = make_label(right, "0 %",
            lv_color_hex(0x555555), &lv_font_montserrat_14);
        lv_obj_align(pct, LV_ALIGN_TOP_RIGHT, -12, bar_y);

        lv_obj_t *fill = make_bar(right, RIGHT_W - 24);
        lv_obj_set_pos(lv_obj_get_parent(fill), 12, bar_y + 16);
        *bar_ptrs[i] = fill;
        bar_y += 36;
    }

    // Divider
    lv_obj_t *rdiv = make_obj(right);
    lv_obj_set_size(rdiv, RIGHT_W, 1);
    lv_obj_set_pos(rdiv, 0, bar_y + 4);
    lv_obj_set_style_bg_color(rdiv, lv_color_hex(0xEEEEEA), 0);
    lv_obj_set_style_bg_opa(rdiv, LV_OPA_COVER, 0);

    lv_obj_t *sys_hdr = make_label(right, "SYSTEM",
        lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
    lv_obj_set_style_text_letter_space(sys_hdr, 2, 0);
    lv_obj_set_pos(sys_hdr, 12, bar_y + 12);

    // System rows
    const char  *sys_keys[] = { "Tasks", "Wifi", "cmd" };
    const char  *sys_vals[] = { "4 / 4", "OK",   "0x00" };
    lv_color_t   sys_cols[] = {
        lv_color_hex(0x222222),
        lv_color_hex(0x2E7D32),
        lv_color_hex(0x999999)
    };
    int32_t sys_y = bar_y + 30;
    for (int i = 0; i < 3; i++) {
        lv_obj_t *k = make_label(right, sys_keys[i],
            lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
        lv_obj_set_pos(k, 12, sys_y);
        lv_obj_t *v = make_label(right, sys_vals[i], sys_cols[i],
            &lv_font_montserrat_14);
        lv_obj_align(v, LV_ALIGN_TOP_RIGHT, -12, sys_y);
        sys_y += 22;
    }
}