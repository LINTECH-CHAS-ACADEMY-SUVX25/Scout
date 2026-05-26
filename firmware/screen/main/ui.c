#include "ui.h"
#include "rc_protocol.h"
#include "lvgl.h"
#include <math.h>

static uint8_t   s_cmd  = CMD_STOP;
static lv_obj_t *s_knob;
static lv_obj_t *s_halo;

uint8_t ui_get_cmd(void)
{
    return s_cmd;
}

static lv_obj_t *create_tick(lv_obj_t *parent, lv_align_t align, int32_t x_ofs, int32_t y_ofs)
{
    lv_obj_t *t = lv_obj_create(parent);
    lv_obj_set_size(t, 2, 10);
    lv_obj_align(t, align, x_ofs, y_ofs);
    lv_obj_set_style_bg_color(t, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_30, 0);
    lv_obj_set_style_radius(t, 1, 0);
    lv_obj_set_style_border_width(t, 0, 0);
    lv_obj_clear_flag(t, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return t;
}

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
        if (mag > 52.0f) 
        {
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

    } else {
        lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);

        lv_obj_set_style_transform_zoom(s_knob, 256, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A3A3A), 0);

        s_cmd = CMD_STOP;
    }
}

void ui_init(void)
{
    // The background color of the screen.
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0);

    // Base circle
    lv_obj_t *base = lv_obj_create(lv_scr_act());
    lv_obj_set_size(base, 160, 160);
    lv_obj_align(base, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(base, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(base, 2, 0);
    lv_obj_set_style_border_color(base, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_opa(base, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(base, 16, 0);
    lv_obj_set_style_shadow_color(base, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(base, LV_OPA_30, 0);
    lv_obj_clear_flag(base, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESSING,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_RELEASED,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESS_LOST, NULL);

    // Inner guide ring
    lv_obj_t *ring = lv_obj_create(base);
    lv_obj_set_size(ring, 100, 100);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring, 1, 0);
    lv_obj_set_style_border_color(ring, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_border_opa(ring, LV_OPA_10, 0);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // tick marks
    create_tick(base, LV_ALIGN_TOP_MID,    0,  6);
    create_tick(base, LV_ALIGN_BOTTOM_MID, 0, -6);

    lv_obj_t *tick_l = lv_obj_create(base);
    lv_obj_set_size(tick_l, 10, 2);
    lv_obj_align(tick_l, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_bg_color(tick_l, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(tick_l, LV_OPA_30, 0);
    lv_obj_set_style_radius(tick_l, 1, 0);
    lv_obj_set_style_border_width(tick_l, 0, 0);
    lv_obj_clear_flag(tick_l, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tick_r = lv_obj_create(base);
    lv_obj_set_size(tick_r, 10, 2);
    lv_obj_align(tick_r, LV_ALIGN_RIGHT_MID, -6, 0);
    lv_obj_set_style_bg_color(tick_r, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(tick_r, LV_OPA_30, 0);
    lv_obj_set_style_radius(tick_r, 1, 0);
    lv_obj_set_style_border_width(tick_r, 0, 0);
    lv_obj_clear_flag(tick_r, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // Direction labels
    const char *dir_sym[4]   = { LV_SYMBOL_UP, LV_SYMBOL_DOWN, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT };
    lv_align_t  dir_align[4] = { LV_ALIGN_TOP_MID, LV_ALIGN_BOTTOM_MID, LV_ALIGN_LEFT_MID, LV_ALIGN_RIGHT_MID };
    int32_t     dir_xofs[4]  = { 0, 0, 12, -12 };
    int32_t     dir_yofs[4]  = { 14, -14, 0, 0 };

    for (int i = 0; i < 4; i++) 
    {
        lv_obj_t *lbl = lv_label_create(base);
        lv_label_set_text(lbl, dir_sym[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_opa(lbl, LV_OPA_30, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl, dir_align[i], dir_xofs[i], dir_yofs[i]);
        lv_obj_clear_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    }

    // Halo
    s_halo = lv_obj_create(base);
    lv_obj_set_size(s_halo, 70, 70);
    lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_halo, lv_color_hex(0x888888), 0);
    lv_obj_set_style_bg_opa(s_halo, LV_OPA_20, 0);
    lv_obj_set_style_border_width(s_halo, 0, 0);
    lv_obj_clear_flag(s_halo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // Le Knobe
    s_knob = lv_obj_create(base);
    lv_obj_set_size(s_knob, 54, 54);
    lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_bg_opa(s_knob, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_knob, 0, 0);
    lv_obj_set_style_shadow_width(s_knob, 10, 0);
    lv_obj_set_style_shadow_color(s_knob, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(s_knob, LV_OPA_40, 0);
    lv_obj_set_style_transform_pivot_x(s_knob, 27, 0);
    lv_obj_set_style_transform_pivot_y(s_knob, 27, 0);
    lv_obj_set_style_transform_zoom(s_knob, 256, 0);
    lv_obj_clear_flag(s_knob, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // Crosshair ring
    lv_obj_t *ch_ring = lv_obj_create(s_knob);
    lv_obj_set_size(ch_ring, 18, 18);
    lv_obj_align(ch_ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ch_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ch_ring, 1, 0);
    lv_obj_set_style_border_color(ch_ring, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_opa(ch_ring, LV_OPA_40, 0);
    lv_obj_clear_flag(ch_ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // Crosshair center dot
    lv_obj_t *ch_dot = lv_obj_create(s_knob);
    lv_obj_set_size(ch_dot, 5, 5);
    lv_obj_align(ch_dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ch_dot, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_opa(ch_dot, LV_OPA_60, 0);
    lv_obj_set_style_border_width(ch_dot, 0, 0);
    lv_obj_clear_flag(ch_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}