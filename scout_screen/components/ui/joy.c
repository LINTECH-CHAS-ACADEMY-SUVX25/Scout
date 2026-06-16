#include "internal.h"
#include "joy.h"
#include "rc_protocol.h"
#include <math.h>

static volatile int16_t s_joy_x;
static volatile int16_t s_joy_y;
static uint8_t          s_last_cmd = 0xFF;
static uint8_t          s_cmd      = CMD_STOP;

static lv_obj_t *s_knob;
static lv_obj_t *s_halo;
static lv_obj_t *s_cmd_badges[5];
static lv_obj_t *s_cmd_badge_lbls[5];

static void update_cmd_badges(uint8_t cmd)
{
    if(cmd == s_last_cmd) return;
    s_last_cmd = cmd;
    s_cmd = cmd;

    static const uint8_t masks[5] = {
        CMD_FORWARD, CMD_BACKWARD, 0xFF, CMD_LEFT, CMD_RIGHT
    };
    for(int i = 0; i < 5; i++) {
        bool active = (i == 2) ? (cmd == CMD_STOP) : (cmd & masks[i]);
        if(active) {
            lv_obj_add_state(s_cmd_badges[i],     LV_STATE_USER_1);
            lv_obj_add_state(s_cmd_badge_lbls[i], LV_STATE_USER_1);
        } else {
            lv_obj_clear_state(s_cmd_badges[i],     LV_STATE_USER_1);
            lv_obj_clear_state(s_cmd_badge_lbls[i], LV_STATE_USER_1);
        }
    }
}

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
        if(mag > JOY_RADIUS_PX) {
            dx = (int)(dx * JOY_RADIUS_PX / mag);
            dy = (int)(dy * JOY_RADIUS_PX / mag);
        }

        lv_obj_align(s_knob, LV_ALIGN_CENTER, dx, dy);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, dx, dy);
        lv_obj_set_style_transform_zoom(s_knob, 210, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(COL_ACCENT), 0);

        if(mag <= JOY_DEADZONE_PX) {
            s_joy_x = 0;
            s_joy_y = 0;
        } else {
            s_joy_x =  (int16_t)((dx * 255) / JOY_RADIUS_PX);
            s_joy_y = -(int16_t)((dy * 255) / JOY_RADIUS_PX);
        }

        uint8_t cmd = CMD_STOP;
        if(dy < -JOY_DEADZONE_PX) cmd |= CMD_FORWARD;
        if(dy >  JOY_DEADZONE_PX) cmd |= CMD_BACKWARD;
        if(dx < -JOY_DEADZONE_PX) cmd |= CMD_LEFT;
        if(dx >  JOY_DEADZONE_PX) cmd |= CMD_RIGHT;
        update_cmd_badges(cmd);
    } else {
        lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_transform_zoom(s_knob, 256, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(KNOB_IDLE), 0);
        s_joy_x = 0;
        s_joy_y = 0;
        update_cmd_badges(CMD_STOP);
    }
}

static lv_obj_t *create_joy_tick(lv_obj_t *parent, lv_align_t align,
                                  int32_t xo, int32_t yo, bool horiz)
{
    lv_obj_t *t = make_obj(parent);
    lv_obj_add_style(t, &st_fill_textlo, 0);
    lv_obj_set_size(t, horiz ? 7 : 1, horiz ? 1 : 7);
    lv_obj_align(t, align, xo, yo);
    lv_obj_set_style_radius(t, 1, 0);
    return t;
}

static void make_cmd_badges(lv_obj_t *joy_panel)
{
    const char *badge_labels[5] = { "FWD", "BWD", "STP", "LFT", "RGT" };
    int32_t badge_x = (SIDE_W - (5 * BADGE_W + 4 * BADGE_GAP)) / 2;
    for(int i = 0; i < 5; i++) {
        lv_obj_t *badge = make_obj(joy_panel);
        lv_obj_add_style(badge, &st_chip,    0);
        lv_obj_add_style(badge, &st_chip_on, LV_STATE_USER_1);
        lv_obj_set_size(badge, BADGE_W, BADGE_H);
        lv_obj_set_pos(badge, badge_x, 44);
        lv_obj_t *bl = make_label(badge, badge_labels[i], &st_fg_lo, NULL);
        lv_obj_add_style(bl, &st_fg_accent, LV_STATE_USER_1);
        lv_obj_align(bl, LV_ALIGN_CENTER, 0, 0);
        s_cmd_badges[i]     = badge;
        s_cmd_badge_lbls[i] = bl;
        badge_x += BADGE_STEP;
    }
    s_last_cmd = 0xFF;
    update_cmd_badges(CMD_STOP);
}

static void make_joystick(lv_obj_t *joy_panel)
{
    lv_obj_t *joy_ring = make_obj(joy_panel);
    lv_obj_add_style(joy_ring, &st_border_line, 0);
    lv_obj_set_size(joy_ring, 146, 146);
    lv_obj_set_pos(joy_ring, (SIDE_W - 146) / 2, 78);
    lv_obj_set_style_radius(joy_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(joy_ring, 1, 0);

    for(int dy = -1; dy <= 1; dy += 2) {
        for(int dx = -1; dx <= 1; dx += 2) {
            lv_obj_t *d = make_obj(joy_ring);
            lv_obj_add_style(d, &st_fill_accent, 0);
            lv_obj_set_size(d, 3, 3);
            lv_obj_set_pos(d, 73 + dx * 51 - 1, 73 + dy * 51 - 1);
            lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(d, LV_OPA_50, 0);
        }
    }

    lv_obj_t *base = lv_obj_create(joy_panel);
    lv_obj_add_style(base, &st_fill_bg, 0);
    lv_obj_add_style(base, &st_border_line, 0);
    lv_obj_set_size(base, 130, 130);
    lv_obj_set_pos(base, (SIDE_W - 130) / 2, 86);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(base, 2, 0);
    // Shadow removed — blurred shadow rendering into PSRAM framebuffers
    // stalled the LVGL render pass and tripped the task watchdog.
    lv_obj_clear_flag(base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESSING,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_RELEASED,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *guide = make_obj(base);
    lv_obj_add_style(guide, &st_border_line, 0);
    lv_obj_set_size(guide, 82, 82);
    lv_obj_align(guide, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(guide, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(guide, 1, 0);

    create_joy_tick(base, LV_ALIGN_TOP_MID,    0,  7, false);
    create_joy_tick(base, LV_ALIGN_BOTTOM_MID, 0, -7, false);
    create_joy_tick(base, LV_ALIGN_LEFT_MID,   7,  0, true);
    create_joy_tick(base, LV_ALIGN_RIGHT_MID, -7,  0, true);

    s_halo = make_obj(base);
    lv_obj_add_style(s_halo, &st_fill_accent, 0);
    lv_obj_set_size(s_halo, 62, 62);
    lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_halo, LV_OPA_20, 0);

    s_knob = make_obj(base);
    lv_obj_set_size(s_knob, 46, 46);
    lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_knob, lv_color_hex(KNOB_IDLE), 0);
    lv_obj_set_style_bg_opa(s_knob, LV_OPA_COVER, 0);
    // Shadow removed — same watchdog/stall reason as the base above.
    lv_obj_set_style_transform_pivot_x(s_knob, 23, 0);
    lv_obj_set_style_transform_pivot_y(s_knob, 23, 0);
    lv_obj_set_style_transform_zoom(s_knob, 256, 0);

    lv_obj_t *ch_ring = make_obj(s_knob);
    lv_obj_add_style(ch_ring, &st_border_texthi, 0);
    lv_obj_set_size(ch_ring, 16, 16);
    lv_obj_align(ch_ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ch_ring, 1, 0);

    lv_obj_t *ch_dot = make_obj(s_knob);
    lv_obj_add_style(ch_dot, &st_fill_texthi, 0);
    lv_obj_set_size(ch_dot, 4, 4);
    lv_obj_align(ch_dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ch_dot, LV_OPA_60, 0);
}

void joy_panel_build(void)
{
    lv_obj_t *joy_panel = make_panel(PANEL_GAP,
                                     CAM_Y + CAM_H - PANEL_H,
                                     SIDE_W, PANEL_H);
    make_section_hdr(joy_panel, "JOYSTICK", HDR_Y);
    make_cmd_badges(joy_panel);
    make_joystick(joy_panel);
}

void scout_ui_get_joy(int16_t *x, int16_t *y)
{
    *x = s_joy_x;
    *y = s_joy_y;
}

