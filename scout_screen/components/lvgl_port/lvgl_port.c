#include "lvgl_port.h"
#include "display.h"
#include "rc_protocol.h"
#include "lvgl.h"
#include "rgb_lcd_port.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

// Connects LVGL to the Waveshare RGB LCD and GT911 touch controller.
// Owns the full UI layout — widget creation, event callbacks, and rendering.

static volatile uint8_t s_cmd = CMD_STOP;

#define JOY_RADIUS 52

// Intro overlay loading bar
#define INTRO_BAR_W 400
#define INTRO_BAR_H 22

// Widget handles

static lv_obj_t *s_intro_overlay;
static lv_obj_t *s_intro_bar_fill;
static lv_obj_t *s_signal_lost_overlay;
static lv_obj_t *s_knob;
static lv_obj_t *s_halo;
static lv_obj_t *s_conn_dot;
static lv_obj_t *s_conn_label;
static lv_obj_t *s_cmd_badges[5];
static lv_obj_t *s_val_temp;
static lv_obj_t *s_val_humi;
static lv_obj_t *s_val_pres;
static lv_obj_t *s_val_uptime;
static lv_obj_t *s_val_cmd_hex;

// LVGL driver callbacks

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    display_draw_bitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(drv);
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t x, y;
    bool touched = display_read_touch(&x, &y);
    data->point.x = x;
    data->point.y = y;
    data->state   = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void tick_task(void *arg)
{
    while(1) {
        lv_tick_inc(10);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Widget helpers

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

    lv_obj_t *v = make_label(row, init_val, val_color, &lv_font_montserrat_14);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, 0, 0);
    return v;
}

// Command badges

static void update_cmd_badges(uint8_t cmd)
{
    static const uint8_t masks[5] = {
        CMD_FORWARD, CMD_BACKWARD, 0xFF, CMD_LEFT, CMD_RIGHT
    };
    for(int i = 0; i < 5; i++) {
        bool active = (i == 2) ? (cmd == CMD_STOP) : (cmd & masks[i]);
        lv_obj_set_style_bg_color(s_cmd_badges[i],
            active ? lv_color_hex(0xE8F4FF) : lv_color_hex(0xEEEEEA), 0);
        lv_obj_set_style_border_color(s_cmd_badges[i],
            active ? lv_color_hex(0xB5D4F4) : lv_color_hex(0xDDDDD8), 0);

        lv_obj_t *lbl = lv_obj_get_child(s_cmd_badges[i], 0);
        if(lbl) {
            lv_obj_set_style_text_color(lbl,
                active ? lv_color_hex(0x185FA5) : lv_color_hex(0xAAAAAA), 0);
        }
    }
}

// Joystick event

static void joy_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t       *base = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t  pt;
        lv_indev_get_point(indev, &pt);

        lv_area_t coords;
        lv_obj_get_coords(base, &coords);
        int dx = pt.x - (coords.x1 + coords.x2) / 2;
        int dy = pt.y - (coords.y1 + coords.y2) / 2;

        float mag = sqrtf((float)(dx * dx + dy * dy));
        if(mag > JOY_RADIUS) {
            dx = (int)(dx * JOY_RADIUS / mag);
            dy = (int)(dy * JOY_RADIUS / mag);
        }

        lv_obj_align(s_knob, LV_ALIGN_CENTER, dx, dy);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, dx, dy);
        lv_obj_set_style_transform_zoom(s_knob, 210, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x222222), 0);

        uint8_t cmd = CMD_STOP;
        if(dy < -15) cmd |= CMD_FORWARD;
        if(dy >  15) cmd |= CMD_BACKWARD;
        if(dx < -15) cmd |= CMD_LEFT;
        if(dx >  15) cmd |= CMD_RIGHT;
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

// UI update

// Future: lvgl_port_ui_update(float temp, float humi, float pres, bool connected)
// when cam sends sensor data — also update ui.c call site
void lvgl_port_ui_update(bool connected)
{
    lv_obj_set_style_bg_color(s_conn_dot,
        connected ? lv_color_hex(0x4CAF50) : lv_color_hex(0xE24B4A), 0);
    lv_label_set_text(s_conn_label, connected ? "connected" : "waiting...");
}

void lvgl_port_signal_lost(bool lost)
{
    if(lost)
        lv_obj_clear_flag(s_signal_lost_overlay, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(s_signal_lost_overlay, LV_OBJ_FLAG_HIDDEN);
}

// UI init — trivially sequential widget creation (~200 lines, accepted exception)

#define XSTR(x) STR(x)
#define STR(x)  #x

static void lvgl_port_ui_init(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Topbar
    lv_obj_t *topbar = make_obj(lv_scr_act());
    lv_obj_set_size(topbar, SCREEN_W, 36);
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

    s_conn_label = make_label(topbar, "väntar...",
        lv_color_hex(0x888888), &lv_font_montserrat_14);
    lv_obj_align(s_conn_label, LV_ALIGN_RIGHT_MID, -90, 0);

    s_conn_dot = make_obj(topbar);
    lv_obj_set_size(s_conn_dot, 6, 6);
    lv_obj_align(s_conn_dot, LV_ALIGN_RIGHT_MID, -170, 0);
    lv_obj_set_style_radius(s_conn_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_conn_dot, lv_color_hex(0xE24B4A), 0);
    lv_obj_set_style_bg_opa(s_conn_dot, LV_OPA_COVER, 0);

    // Bottombar
    lv_obj_t *botbar = make_obj(lv_scr_act());
    lv_obj_set_size(botbar, SCREEN_W, 32);
    lv_obj_align(botbar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(botbar, lv_color_hex(0x111417), 0);
    lv_obj_set_style_bg_opa(botbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(botbar, 0, 0);
    lv_obj_set_style_border_side(botbar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(botbar, lv_color_hex(0x1C1F24), 0);

    const char *bot_keys[] = { "IP", "TCP" };
    const char *bot_vals[] = { S3_IP, XSTR(VID_PORT) };
    const int32_t key_w[]  = { 24, 38 };
    int32_t bot_x = 14;
    for(int i = 0; i < 2; i++) {
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

    const int32_t CONTENT_Y = 36;
    const int32_t CONTENT_H = SCREEN_H - 36 - 32;
    const int32_t LEFT_W    = 200;
    const int32_t RIGHT_W   = 160;
    const int32_t VIDEO_W   = SCREEN_W - LEFT_W - RIGHT_W;

    // Left panel
    lv_obj_t *left = make_obj(lv_scr_act());
    lv_obj_set_size(left, LEFT_W, CONTENT_H);
    lv_obj_set_pos(left, 0, CONTENT_Y);
    lv_obj_set_style_bg_color(left, lv_color_hex(0xF7F7F5), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(left, 1, 0);
    lv_obj_set_style_border_side(left, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(left, lv_color_hex(0xE8E8E4), 0);
    lv_obj_set_style_radius(left, 0, 0);

    lv_obj_t *tele_hdr = make_label(left, "TELEMETRI",
        lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
    lv_obj_set_style_text_letter_space(tele_hdr, 2, 0);
    lv_obj_align(tele_hdr, LV_ALIGN_TOP_LEFT, 14, 10);

    lv_obj_t *tele_cont = make_obj(left);
    lv_obj_set_size(tele_cont, 172, 110);
    lv_obj_set_pos(tele_cont, 14, 28);

    s_val_cmd_hex = make_tele_row(tele_cont, "cmd",         "0x00",     lv_color_hex(0x999999),   0);
    s_val_temp    = make_tele_row(tele_cont, "temperature", "--.- C",   lv_color_hex(0x222222),  22);
    s_val_humi    = make_tele_row(tele_cont, "humidity",    "-- %",     lv_color_hex(0x222222),  44);
    s_val_pres    = make_tele_row(tele_cont, "pressure",    "---- hPa", lv_color_hex(0x222222),  66);
    s_val_uptime  = make_tele_row(tele_cont, "uptime",      "00:00:00", lv_color_hex(0x222222),  88);

    lv_obj_t *divider = make_obj(left);
    lv_obj_set_size(divider, LEFT_W, 1);
    lv_obj_set_pos(divider, 0, CONTENT_H - 210);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0xE8E8E4), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

    lv_obj_t *joy_hdr = make_label(left, "JOYSTICK",
        lv_color_hex(0xAAAAAA), &lv_font_montserrat_14);
    lv_obj_set_style_text_letter_space(joy_hdr, 2, 0);
    lv_obj_set_pos(joy_hdr, 14, CONTENT_H - 200);

    lv_obj_t *base = lv_obj_create(left);
    lv_obj_set_size(base, 130, 130);
    lv_obj_set_pos(base, (LEFT_W - 130) / 2, CONTENT_H - 180);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(base, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(base, 2, 0);
    lv_obj_set_style_border_color(base, lv_color_hex(0x2E2E2E), 0);
    lv_obj_set_style_border_opa(base, LV_OPA_COVER, 0);
    // Shadow removed — blurred shadow rendering into PSRAM framebuffers
    // stalled the LVGL render pass and tripped the task watchdog.
    lv_obj_clear_flag(base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESSING,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_RELEASED,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *guide = lv_obj_create(base);
    lv_obj_set_size(guide, 82, 82);
    lv_obj_align(guide, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(guide, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(guide, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(guide, 1, 0);
    lv_obj_set_style_border_color(guide, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_border_opa(guide, LV_OPA_COVER, 0);
    lv_obj_clear_flag(guide, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    create_joy_tick(base, LV_ALIGN_TOP_MID,    0,  7, false);
    create_joy_tick(base, LV_ALIGN_BOTTOM_MID, 0, -7, false);
    create_joy_tick(base, LV_ALIGN_LEFT_MID,   7,  0, true);
    create_joy_tick(base, LV_ALIGN_RIGHT_MID, -7,  0, true);

    s_halo = lv_obj_create(base);
    lv_obj_set_size(s_halo, 62, 62);
    lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_halo, lv_color_hex(0x888888), 0);
    lv_obj_set_style_bg_opa(s_halo, LV_OPA_20, 0);
    lv_obj_set_style_border_width(s_halo, 0, 0);
    lv_obj_clear_flag(s_halo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    s_knob = lv_obj_create(base);
    lv_obj_set_size(s_knob, 46, 46);
    lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_bg_opa(s_knob, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_knob, 0, 0);
    // Shadow removed — same watchdog/stall reason as the base above.
    lv_obj_set_style_transform_pivot_x(s_knob, 23, 0);
    lv_obj_set_style_transform_pivot_y(s_knob, 23, 0);
    lv_obj_set_style_transform_zoom(s_knob, 256, 0);
    lv_obj_clear_flag(s_knob, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ch_ring = lv_obj_create(s_knob);
    lv_obj_set_size(ch_ring, 16, 16);
    lv_obj_align(ch_ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ch_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ch_ring, 1, 0);
    lv_obj_set_style_border_color(ch_ring, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_opa(ch_ring, LV_OPA_40, 0);
    lv_obj_clear_flag(ch_ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ch_dot = lv_obj_create(s_knob);
    lv_obj_set_size(ch_dot, 4, 4);
    lv_obj_align(ch_dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ch_dot, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_opa(ch_dot, LV_OPA_60, 0);
    lv_obj_set_style_border_width(ch_dot, 0, 0);
    lv_obj_clear_flag(ch_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    const char *badge_labels[5] = { "FWD", "BWD", "STP", "LFT", "RGT" };
    int32_t badge_x = 6;
    for(int i = 0; i < 5; i++) {
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

    // Right panel
    lv_obj_t *right = make_obj(lv_scr_act());
    lv_obj_set_size(right, RIGHT_W, CONTENT_H);
    lv_obj_set_pos(right, LEFT_W + VIDEO_W, CONTENT_Y);
    lv_obj_set_style_bg_color(right, lv_color_hex(0xF7F7F5), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(right, 1, 0);
    lv_obj_set_style_border_side(right, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(right, lv_color_hex(0xE8E8E4), 0);
    lv_obj_set_style_radius(right, 0, 0);

    s_signal_lost_overlay = make_obj(lv_scr_act());
    lv_obj_set_size(s_signal_lost_overlay, VIDEO_W, CONTENT_H);
    lv_obj_set_pos(s_signal_lost_overlay, LEFT_W, CONTENT_Y);
    lv_obj_set_style_bg_color(s_signal_lost_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_signal_lost_overlay, LV_OPA_COVER, 0);
    lv_obj_add_flag(s_signal_lost_overlay, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *sig_lbl = make_label(s_signal_lost_overlay, "SIGNAL LOST",
        lv_color_hex(0xFFFFFF), &lv_font_montserrat_48);
    lv_obj_set_style_text_letter_space(sig_lbl, 6, 0);
    lv_obj_align(sig_lbl, LV_ALIGN_CENTER, 0, 0);
}

// Driver init

void lvgl_port_init(void)
{
    lv_init();

    lv_color_t *lvgl_buf = heap_caps_malloc(
        SCREEN_W * 20 * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(lvgl_buf);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, lvgl_buf, NULL, SCREEN_W * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res      = SCREEN_W;
    disp_drv.ver_res      = SCREEN_H;
    disp_drv.flush_cb     = flush_cb;
    disp_drv.draw_buf     = &draw_buf;
    disp_drv.full_refresh = 0;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    xTaskCreatePinnedToCore(tick_task, "lvgl_tick", 2048, NULL, 5, NULL, 1);

    lvgl_port_ui_init();
}

uint8_t lvgl_port_get_cmd(void) { return s_cmd; }

// Intro screen

static void intro_bar_exec(void *var, int32_t val)
{
    lv_obj_set_width((lv_obj_t *)var, val);
}

static void intro_anim_done(lv_anim_t *a)
{
    (void)a;
    lv_obj_del(s_intro_overlay);
    s_intro_overlay = NULL;
}

void lvgl_port_intro_screen(void)
{
    // Overlay on the main screen — avoids lv_scr_load framebuffer issues
    s_intro_overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_intro_overlay, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(s_intro_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_intro_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_intro_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_intro_overlay, 0, 0);
    lv_obj_set_style_radius(s_intro_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_intro_overlay, 0, 0);
    lv_obj_clear_flag(s_intro_overlay, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *logo = lv_label_create(s_intro_overlay);
    lv_label_set_text(logo, "SCOUT");
    lv_obj_set_style_text_color(logo, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(logo, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_letter_space(logo, 20, 0);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, -60);

    lv_obj_t *track = lv_obj_create(s_intro_overlay);
    lv_obj_set_size(track, INTRO_BAR_W, INTRO_BAR_H);
    lv_obj_align(track, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(track, 2, 0);
    lv_obj_set_style_border_color(track, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(track, 0, 0);
    lv_obj_set_style_pad_all(track, 0, 0);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    s_intro_bar_fill = lv_obj_create(track);
    lv_obj_set_size(s_intro_bar_fill, 0, INTRO_BAR_H);
    lv_obj_set_pos(s_intro_bar_fill, 0, 0);
    lv_obj_set_style_bg_color(s_intro_bar_fill, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(s_intro_bar_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_intro_bar_fill, 0, 0);
    lv_obj_set_style_radius(s_intro_bar_fill, 0, 0);
    lv_obj_set_style_pad_all(s_intro_bar_fill, 0, 0);
    lv_obj_clear_flag(s_intro_bar_fill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *loading_lbl = lv_label_create(s_intro_overlay);
    lv_label_set_text(loading_lbl, "LOADING...");
    lv_obj_set_style_text_color(loading_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(loading_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(loading_lbl, 4, 0);
    lv_obj_align(loading_lbl, LV_ALIGN_CENTER, 0, 84);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_intro_bar_fill);
    lv_anim_set_exec_cb(&a, intro_bar_exec);
    lv_anim_set_values(&a, 0, INTRO_BAR_W);
    lv_anim_set_time(&a, 2500);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a, intro_anim_done);
    lv_anim_start(&a);
}

// Render

void lvgl_port_render_frame(void)
{
    lv_timer_t *refr = lv_disp_get_default()->refr_timer;
    lv_timer_pause(refr);
    lv_timer_handler();              // input phase: indev fires, dirty areas accumulate
    lv_timer_resume(refr);
    lv_refr_now(lv_disp_get_default()); // render all accumulated dirty areas immediately
}
