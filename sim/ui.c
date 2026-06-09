// UI-vy för Scout-skärmen — SPEGLING av UI-sektionen i
// scout_screen/components/lvgl_port/lvgl_port.c.
//
// Allt mellan "KOPIERA FRÅN HÄR" och "KOPIERA TILL HÄR" är ordagrant
// identiskt med enhetskoden och kan klistras tillbaka rakt in i
// lvgl_port.c. Raderna utanför markörerna (dessa includes + sim_ui_init
// längst ned) är simulator-specifika och ska INTE kopieras.

#include "lvgl.h"
#include "rc_protocol.h"
#include "sim_screen.h"   // SCREEN_W / SCREEN_H (motsvarar display.h på enheten)
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

// ===================== KOPIERA FRÅN HÄR =============================

#define JOY_RADIUS 52

// Layout — två lika breda sidofält flankerar en centrerad kameruta.
// BAR_H är lika för topp och botten så kameran blir vertikalt symmetrisk.
#define BAR_H       36
#define CONTENT_Y   BAR_H
#define CONTENT_H   (SCREEN_H - 2 * BAR_H)
#define SIDE_W      240
#define PAD         14
#define ROW_W       (SIDE_W - 2 * PAD)
#define TELE_HDR_Y  12                  // rubriken högst upp i sidofältet
#define TELE_ROWS_Y 94                  // värdesraderna nedflyttade till bra höjd
#define TELE_PITCH  30                  // radavstånd i telemetrin

// Styrknappar (FWD/BWD/STP/LFT/RGT)
#define BADGE_W     38
#define BADGE_H     20
#define BADGE_GAP   7
#define BADGE_STEP  (BADGE_W + BADGE_GAP)

// Palett — dark tech / premium
#define COL_BG         0x0A0E14
#define COL_BAR        0x0D1117
#define COL_PANEL      0x141A23
#define COL_LINE       0x232B36
#define COL_ACCENT     0x22D3EE
#define COL_TEXT_HI    0xE6EDF3
#define COL_TEXT_MID   0x9BA7B4
#define COL_TEXT_LO    0x5C6773
#define COL_GOOD       0x34D399
#define COL_BAD        0xF87171
#define COL_BADGE_BG   0x1B2230
#define COL_BADGE_ON   0x10222A

// UI-font — Press Start 2P (genererad C-font, se press_start_2p_8.c)
LV_FONT_DECLARE(press_start_2p_8);
#define UI_FONT (&press_start_2p_8)

static volatile uint8_t s_cmd = CMD_STOP;

// Widget handles

static lv_obj_t *s_knob;
static lv_obj_t *s_halo;
static lv_obj_t *s_conn_dot;
static lv_obj_t *s_conn_label;
static lv_obj_t *s_cmd_badges[5];
static lv_obj_t *s_val_temp;
static lv_obj_t *s_val_humi;
static lv_obj_t *s_val_pres;

// Widget helpers

static lv_obj_t *make_obj(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
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

static lv_obj_t *make_bar(lv_obj_t *parent, int32_t w)
{
    lv_obj_t *track = make_obj(parent);
    lv_obj_set_size(track, w, 3);
    lv_obj_set_style_bg_color(track, lv_color_hex(COL_LINE), 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(track, 2, 0);

    lv_obj_t *fill = make_obj(track);
    lv_obj_set_size(fill, 0, 3);
    lv_obj_align(fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(fill, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(fill, 2, 0);
    return fill;
}

// Vänster sidomeny — enda panelen. Linjen sitter på högerkanten.
static lv_obj_t *make_sidebar(void)
{
    lv_obj_t *p = make_obj(lv_scr_act());
    lv_obj_set_size(p, SIDE_W, CONTENT_H);
    lv_obj_set_pos(p, 0, CONTENT_Y);
    lv_obj_set_style_bg_color(p, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(p, 1, 0);
    lv_obj_set_style_border_side(p, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(COL_LINE), 0);
    return p;
}

// Sektionsrubrik med cyan accent-markör till vänster.
static void make_section_hdr(lv_obj_t *parent, const char *text, int32_t y)
{
    lv_obj_t *mark = make_obj(parent);
    lv_obj_set_size(mark, 3, 14);
    lv_obj_set_pos(mark, PAD, y + 1);
    lv_obj_set_style_radius(mark, 1, 0);
    lv_obj_set_style_bg_color(mark, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_opa(mark, LV_OPA_COVER, 0);

    lv_obj_t *l = make_label(parent, text,
        lv_color_hex(COL_TEXT_MID), UI_FONT);
    lv_obj_set_style_text_letter_space(l, 2, 0);
    lv_obj_set_pos(l, PAD + 10, y);
}

static lv_obj_t *create_joy_tick(lv_obj_t *parent, lv_align_t align,
                                   int32_t xo, int32_t yo, bool horiz)
{
    lv_obj_t *t = lv_obj_create(parent);
    lv_obj_set_size(t, horiz ? 7 : 1, horiz ? 1 : 7);
    lv_obj_align(t, align, xo, yo);
    lv_obj_set_style_bg_color(t, lv_color_hex(COL_TEXT_LO), 0);
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
    lv_obj_set_size(row, ROW_W, 22);
    lv_obj_set_pos(row, 0, y);

    lv_obj_t *k = make_label(row, key,
        lv_color_hex(COL_TEXT_MID), UI_FONT);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *v = make_label(row, init_val, val_color, UI_FONT);
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
            lv_color_hex(active ? COL_BADGE_ON : COL_BADGE_BG), 0);
        lv_obj_set_style_border_color(s_cmd_badges[i],
            lv_color_hex(active ? COL_ACCENT : COL_LINE), 0);

        lv_obj_t *lbl = lv_obj_get_child(s_cmd_badges[i], 0);
        if(lbl) {
            lv_obj_set_style_text_color(lbl,
                lv_color_hex(active ? COL_ACCENT : COL_TEXT_LO), 0);
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
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(COL_ACCENT), 0);

        uint8_t cmd = CMD_STOP;
        if(dy < -15) cmd |= CMD_FORWARD;
        if(dy >  15) cmd |= CMD_BACKWARD;
        if(dx < -15) cmd |= CMD_LEFT;
        if(dx >  15) cmd |= CMD_RIGHT;
        s_cmd = cmd;
        update_cmd_badges(s_cmd);
    } else {
        lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_transform_zoom(s_knob, 256, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A434F), 0);
        s_cmd = CMD_STOP;
        update_cmd_badges(CMD_STOP);
    }
}

// UI update

// Future: lvgl_port_ui_update(float temp, float humi, float pres, bool connected)
// when cam sends sensor data — also update ui.c call site
void lvgl_port_ui_update(bool connected)
{
    lv_obj_set_style_bg_color(s_conn_dot,
        lv_color_hex(connected ? COL_GOOD : COL_BAD), 0);
    lv_label_set_text(s_conn_label, connected ? "connected" : "waiting...");
}

// UI init — trivially sequential widget creation (~200 lines, accepted exception)

#define XSTR(x) STR(x)
#define STR(x)  #x

static void lvgl_port_ui_init(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Topbar
    lv_obj_t *topbar = make_obj(lv_scr_act());
    lv_obj_set_size(topbar, SCREEN_W, BAR_H);
    lv_obj_align(topbar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(topbar, lv_color_hex(COL_BAR), 0);
    lv_obj_set_style_bg_opa(topbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(topbar, 1, 0);
    lv_obj_set_style_border_side(topbar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(topbar, lv_color_hex(COL_LINE), 0);

    lv_obj_t *logo = make_label(topbar, "SCOUT",
        lv_color_hex(COL_ACCENT), UI_FONT);
    lv_obj_set_style_text_letter_space(logo, 6, 0);
    lv_obj_align(logo, LV_ALIGN_LEFT_MID, 14, 0);

    lv_obj_t *sub = make_label(topbar, "RC-TELEMETRI",
        lv_color_hex(COL_TEXT_LO), UI_FONT);
    lv_obj_set_style_text_letter_space(sub, 2, 0);
    lv_obj_align(sub, LV_ALIGN_LEFT_MID, 120, 0);

    s_conn_label = make_label(topbar, "waiting...",
        lv_color_hex(COL_TEXT_MID), UI_FONT);
    lv_obj_align(s_conn_label, LV_ALIGN_RIGHT_MID, -60, 0);

    s_conn_dot = make_obj(topbar);
    lv_obj_set_size(s_conn_dot, 6, 6);
    lv_obj_align(s_conn_dot, LV_ALIGN_RIGHT_MID, -170, 0);
    lv_obj_set_style_radius(s_conn_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_conn_dot, lv_color_hex(COL_BAD), 0);
    lv_obj_set_style_bg_opa(s_conn_dot, LV_OPA_COVER, 0);

    // Bottombar
    lv_obj_t *botbar = make_obj(lv_scr_act());
    lv_obj_set_size(botbar, SCREEN_W, BAR_H);
    lv_obj_align(botbar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(botbar, lv_color_hex(COL_BAR), 0);
    lv_obj_set_style_bg_opa(botbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(botbar, 1, 0);
    lv_obj_set_style_border_side(botbar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(botbar, lv_color_hex(COL_LINE), 0);

    const char *bot_keys[] = { "IP", "UDP" };
    const char *bot_vals[] = { S3_IP, XSTR(VID_PORT) };
    const int32_t key_w[]  = { 24, 38 };
    int32_t bot_x = 14;
    for(int i = 0; i < 2; i++) {
        lv_obj_t *k = make_label(botbar, bot_keys[i],
            lv_color_hex(COL_TEXT_LO), UI_FONT);
        lv_obj_align(k, LV_ALIGN_LEFT_MID, bot_x, 0);
        bot_x += key_w[i];
        lv_obj_t *v = make_label(botbar, bot_vals[i],
            lv_color_hex(COL_TEXT_MID), UI_FONT);
        lv_obj_align(v, LV_ALIGN_LEFT_MID, bot_x, 0);
        bot_x += (i == 0) ? 110 : 50;
    }
    lv_obj_t *rtos_k = make_label(botbar, "RTOS",
        lv_color_hex(COL_TEXT_LO), UI_FONT);
    lv_obj_align(rtos_k, LV_ALIGN_RIGHT_MID, -120, 0);
    lv_obj_t *rtos_v = make_label(botbar, "FREERTOS",
        lv_color_hex(COL_TEXT_MID), UI_FONT);
    lv_obj_align(rtos_v, LV_ALIGN_RIGHT_MID, -14, 0);

    // Enda sidomenyn ligger till vänster. Höger sida lämnas som ren
    // bakgrund — ingen panel, ingen linje.
    lv_obj_t *left = make_sidebar();

    make_section_hdr(left, "TELEMETRI", TELE_HDR_Y);

    lv_obj_t *tele_cont = make_obj(left);
    lv_obj_set_size(tele_cont, ROW_W, 3 * TELE_PITCH);
    lv_obj_set_pos(tele_cont, PAD, TELE_ROWS_Y);

    s_val_temp = make_tele_row(tele_cont, "temperature", "--.- C",   lv_color_hex(COL_TEXT_HI), 0 * TELE_PITCH);
    s_val_humi = make_tele_row(tele_cont, "humidity",    "-- %",     lv_color_hex(COL_TEXT_HI), 1 * TELE_PITCH);
    s_val_pres = make_tele_row(tele_cont, "pressure",    "---- hPa", lv_color_hex(COL_TEXT_HI), 2 * TELE_PITCH);

    lv_obj_t *divider = make_obj(left);
    lv_obj_set_size(divider, SIDE_W, 1);
    lv_obj_set_pos(divider, 0, CONTENT_H - 226);
    lv_obj_set_style_bg_color(divider, lv_color_hex(COL_LINE), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

    make_section_hdr(left, "JOYSTICK", CONTENT_H - 216);

    lv_obj_t *base = lv_obj_create(left);
    lv_obj_set_size(base, 130, 130);
    lv_obj_set_pos(base, (SIDE_W - 130) / 2, CONTENT_H - 180);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(base, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(base, 2, 0);
    lv_obj_set_style_border_color(base, lv_color_hex(COL_LINE), 0);
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
    lv_obj_set_style_border_color(guide, lv_color_hex(COL_LINE), 0);
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
    lv_obj_set_style_bg_color(s_halo, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_opa(s_halo, LV_OPA_20, 0);
    lv_obj_set_style_border_width(s_halo, 0, 0);
    lv_obj_clear_flag(s_halo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    s_knob = lv_obj_create(base);
    lv_obj_set_size(s_knob, 46, 46);
    lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A434F), 0);
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
    lv_obj_set_style_border_color(ch_ring, lv_color_hex(COL_TEXT_HI), 0);
    lv_obj_set_style_border_opa(ch_ring, LV_OPA_40, 0);
    lv_obj_clear_flag(ch_ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ch_dot = lv_obj_create(s_knob);
    lv_obj_set_size(ch_dot, 4, 4);
    lv_obj_align(ch_dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ch_dot, lv_color_hex(COL_TEXT_HI), 0);
    lv_obj_set_style_bg_opa(ch_dot, LV_OPA_60, 0);
    lv_obj_set_style_border_width(ch_dot, 0, 0);
    lv_obj_clear_flag(ch_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    const char *badge_labels[5] = { "FWD", "BWD", "STP", "LFT", "RGT" };
    int32_t badge_x = (SIDE_W - (5 * BADGE_W + 4 * BADGE_GAP)) / 2;   // centrera raden
    for(int i = 0; i < 5; i++) {
        lv_obj_t *badge = lv_obj_create(left);
        lv_obj_set_size(badge, BADGE_W, BADGE_H);
        lv_obj_set_pos(badge, badge_x, CONTENT_H - 38);
        lv_obj_set_style_radius(badge, 3, 0);
        lv_obj_set_style_bg_color(badge, lv_color_hex(COL_BADGE_BG), 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(badge, 1, 0);
        lv_obj_set_style_border_color(badge, lv_color_hex(COL_LINE), 0);
        lv_obj_set_style_pad_all(badge, 0, 0);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *bl = make_label(badge, badge_labels[i],
            lv_color_hex(COL_TEXT_LO), UI_FONT);
        lv_obj_align(bl, LV_ALIGN_CENTER, 0, 0);
        s_cmd_badges[i] = badge;
        badge_x += BADGE_STEP;
    }
    update_cmd_badges(CMD_STOP);
}

// ===================== KOPIERA TILL HÄR =============================

// Simulator-ingång — anropar den statiska ui-init. Kopieras INTE till enheten.
void sim_ui_init(void)
{
    lvgl_port_ui_init();
}
