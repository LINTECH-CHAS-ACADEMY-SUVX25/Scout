/**
 * @file ui_dashboard.c
 * @brief Scout LVGL Dashboard – 1024×600 Waveshare 7" layout
 *
 * Left  640×600 : camera 640×480 (top) + sensor chip bar 640×120 (bottom)
 * Right 384×600 : telemetry panels + joystick + action buttons
 */

#include "ui_dashboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ═══════════════════════════════════════════
 *  COLOUR PALETTE
 * ═══════════════════════════════════════════ */
#define C_BG         lv_color_hex(0x060910)
#define C_PANEL      lv_color_hex(0x0a0e1a)
#define C_CARD       lv_color_hex(0x0f1420)
#define C_BORDER     lv_color_hex(0x151d2e)
#define C_TEXT       lv_color_hex(0xc8d6e5)
#define C_DIM        lv_color_hex(0x5a6a7a)
#define C_ACCENT     lv_color_hex(0x5ba4d9)
#define C_OK         lv_color_hex(0x2ecc71)
#define C_WARN       lv_color_hex(0xf39c12)
#define C_DANGER     lv_color_hex(0xe74c3c)

/* ═══════════════════════════════════════════
 *  WIDGET HANDLES
 * ═══════════════════════════════════════════ */

/* ── top HUD (over camera) ── */
static lv_obj_t *hud_bar;
static lv_obj_t *hud_alert_dot;
static lv_obj_t *hud_temp_lbl;
static lv_obj_t *btn_dropdown;
static lv_obj_t *hud_rec_lbl;

/* ── camera placeholder ── */
static lv_obj_t *cam_area;

/* ── dropdown overlay (slides over camera) ── */
static lv_obj_t *dropdown_panel;
static bool      dropdown_open = false;
/* dropdown labels */
static lv_obj_t *dd_temp, *dd_hum, *dd_press;
static lv_obj_t *dd_co, *dd_co2, *dd_tvoc;
static lv_obj_t *dd_pm25, *dd_pm10, *dd_lux;

/* ── bottom bar sensor chips ── */
static lv_obj_t *chip_co_val, *chip_co2_val, *chip_pm25_val, *chip_dist_val, *chip_lux_val;
static lv_obj_t *chip_co_box, *chip_co2_box, *chip_pm25_box, *chip_dist_box;
/* bottom bar readout */
static lv_obj_t *lbl_steer, *lbl_throttle;

/* ── right panel ── */
static lv_obj_t *rp_time_lbl, *rp_bat_lbl;
/* env */
static lv_obj_t *rp_temp, *rp_hum, *rp_press;
/* gas */
static lv_obj_t *rp_co, *rp_co2, *rp_tvoc;
/* thermal canvas */
static lv_obj_t  *rp_thermal_canvas;
static lv_color_t thermal_buf[64];  /* 8×8 */
static lv_obj_t  *rp_tmin_lbl, *rp_tmax_lbl;
/* imu */
static lv_obj_t *rp_roll, *rp_pitch;
static lv_obj_t *rp_compass;       /* canvas for needle */
static lv_color_t compass_buf[52 * 52];
/* particles */
static lv_obj_t *rp_pm25, *rp_pm10;
/* range */
static lv_obj_t *rp_dist, *rp_lux_r;
/* alert bar */
static lv_obj_t *rp_alert_bar;
static lv_obj_t *rp_alert_lbl;
static lv_obj_t *rp_alert_ev;

/* ── joystick (visual only – input comes from SDL mouse) ── */
static lv_obj_t *joy_ring;
static lv_obj_t *joy_knob;

/* ── action buttons ── */
static lv_obj_t *btn_lights, *btn_rec, *btn_snap;
static bool lights_on  = false;
static bool recording   = true;

/* ═══════════════════════════════════════════
 *  HELPERS
 * ═══════════════════════════════════════════ */

static lv_color_t temp_to_color(float t, float tmin, float tmax)
{
    if (tmax <= tmin) tmax = tmin + 1.0f;
    float r = (t - tmin) / (tmax - tmin);
    if (r < 0) r = 0; if (r > 1) r = 1;
    uint8_t R, G, B;
    if (r < 0.33f)      { float s = r / 0.33f;           R = 0;               G = (uint8_t)(s * 255); B = (uint8_t)((1-s)*255); }
    else if (r < 0.66f) { float s = (r - 0.33f) / 0.33f; R = (uint8_t)(s*255); G = 255;               B = 0; }
    else                 { float s = (r - 0.66f) / 0.34f; R = 255;              G = (uint8_t)((1-s)*255); B = 0; }
    return lv_color_make(R, G, B);
}

static lv_color_t alert_to_color(alert_level_t a)
{
    switch (a) {
        case ALERT_DANGER: return C_DANGER;
        case ALERT_WARN:   return C_WARN;
        default:           return C_OK;
    }
}

/* Create a small card with a title inside a parent at absolute pos */
static lv_obj_t *make_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                           lv_coord_t w, lv_coord_t h, const char *title)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_set_style_bg_color(c, C_CARD, 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(c, 6, 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_border_color(c, C_BORDER, 0);
    lv_obj_set_style_pad_all(c, 5, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    if (title) {
        lv_obj_t *t = lv_label_create(c);
        lv_label_set_text(t, title);
        lv_obj_set_style_text_color(t, C_ACCENT, 0);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_10, 0);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 0, 0);
    }
    return c;
}

/* Label helper: create a value label at (x, y) inside parent */
static lv_obj_t *val_label(lv_obj_t *parent, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, "--");
    lv_obj_set_style_text_color(l, C_TEXT, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(l, x, y);
    return l;
}

/* Set a value label with colour based on thresholds */
static void set_val(lv_obj_t *lbl, const char *fmt, float val,
                    float warn_thr, float danger_thr, bool higher_is_bad)
{
    char buf[48];
    snprintf(buf, sizeof(buf), fmt, val);
    lv_label_set_text(lbl, buf);

    lv_color_t col = C_TEXT;
    if (higher_is_bad) {
        if (danger_thr > 0 && val > danger_thr)  col = C_DANGER;
        else if (warn_thr > 0 && val > warn_thr) col = C_WARN;
    } else {
        if (danger_thr > 0 && val < danger_thr)  col = C_DANGER;
        else if (warn_thr > 0 && val < warn_thr) col = C_WARN;
    }
    lv_obj_set_style_text_color(lbl, col, 0);
}

/* ═══════════════════════════════════════════
 *  DROPDOWN TOGGLE CALLBACK
 * ═══════════════════════════════════════════ */
static void dropdown_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_dashboard_toggle_dropdown();
}

/* ═══════════════════════════════════════════
 *  ACTION BUTTON CALLBACKS
 * ═══════════════════════════════════════════ */
static void lights_cb(lv_event_t *e)
{
    (void)e;
    lights_on = !lights_on;
    lv_obj_set_style_bg_color(btn_lights,
        lights_on ? lv_color_hex(0x2a2510) : C_CARD, 0);
    lv_obj_set_style_border_color(btn_lights,
        lights_on ? lv_color_hex(0xf1c40f) : C_BORDER, 0);
}

static void rec_cb(lv_event_t *e)
{
    (void)e;
    recording = !recording;
    lv_obj_set_style_bg_color(btn_rec,
        recording ? lv_color_hex(0x2a1010) : C_CARD, 0);
    lv_label_set_text(hud_rec_lbl, recording ? "REC" : "");
}

/* ═══════════════════════════════════════════
 *  CREATE
 * ═══════════════════════════════════════════ */
void ui_dashboard_create(lv_obj_t *parent)
{
    /* root */
    lv_obj_set_style_bg_color(parent, C_BG, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* ════════════════════════════════════════
     *  LEFT SIDE  (640 × 600)
     * ════════════════════════════════════════ */

    /* ── Camera area 640×480 ── */
    cam_area = lv_obj_create(parent);
    lv_obj_set_pos(cam_area, 0, 0);
    lv_obj_set_size(cam_area, CAM_W, CAM_H);
    lv_obj_set_style_bg_color(cam_area, lv_color_hex(0x050508), 0);
    lv_obj_set_style_bg_opa(cam_area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cam_area, 0, 0);
    lv_obj_set_style_border_width(cam_area, 0, 0);
    lv_obj_clear_flag(cam_area, LV_OBJ_FLAG_SCROLLABLE);

    /* Placeholder text for simulated feed */
    lv_obj_t *cam_lbl = lv_label_create(cam_area);
    lv_label_set_text(cam_lbl, "ESP32-CAM  640 x 480\n\n"
                               LV_SYMBOL_IMAGE "  LIVE FEED\n\n"
                               "[ Ersatt med riktig strom\n"
                               "  via http://<esp-ip>:81/stream ]");
    lv_obj_set_style_text_color(cam_lbl, lv_color_hex(0x2a3a4a), 0);
    lv_obj_set_style_text_font(cam_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(cam_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(cam_lbl);

    /* Crosshair on camera */
    static lv_point_t ch_h[] = {{CAM_W/2 - 20, CAM_H/2}, {CAM_W/2 + 20, CAM_H/2}};
    static lv_point_t ch_v[] = {{CAM_W/2, CAM_H/2 - 20}, {CAM_W/2, CAM_H/2 + 20}};
    lv_obj_t *lh = lv_line_create(cam_area);
    lv_line_set_points(lh, ch_h, 2);
    lv_obj_set_style_line_color(lh, lv_color_hex(0x5ba4d9), 0);
    lv_obj_set_style_line_opa(lh, LV_OPA_30, 0);
    lv_obj_set_style_line_width(lh, 1, 0);
    lv_obj_t *lv = lv_line_create(cam_area);
    lv_line_set_points(lv, ch_v, 2);
    lv_obj_set_style_line_color(lv, lv_color_hex(0x5ba4d9), 0);
    lv_obj_set_style_line_opa(lv, LV_OPA_30, 0);
    lv_obj_set_style_line_width(lv, 1, 0);

    /* ── HUD bar on top of camera ── */
    hud_bar = lv_obj_create(cam_area);
    lv_obj_set_pos(hud_bar, 0, 0);
    lv_obj_set_size(hud_bar, CAM_W, 30);
    lv_obj_set_style_bg_color(hud_bar, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(hud_bar, LV_OPA_60, 0);
    lv_obj_set_style_radius(hud_bar, 0, 0);
    lv_obj_set_style_border_width(hud_bar, 0, 0);
    lv_obj_clear_flag(hud_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Alert dot */
    hud_alert_dot = lv_obj_create(hud_bar);
    lv_obj_set_size(hud_alert_dot, 8, 8);
    lv_obj_set_style_radius(hud_alert_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hud_alert_dot, C_OK, 0);
    lv_obj_set_style_bg_opa(hud_alert_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hud_alert_dot, 0, 0);
    lv_obj_align(hud_alert_dot, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *title = lv_label_create(hud_bar);
    lv_label_set_text(title, "SCOUT");
    lv_obj_set_style_text_color(title, C_ACCENT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 24, 0);

    hud_rec_lbl = lv_label_create(hud_bar);
    lv_label_set_text(hud_rec_lbl, "REC");
    lv_obj_set_style_text_color(hud_rec_lbl, C_DANGER, 0);
    lv_obj_set_style_text_font(hud_rec_lbl, &lv_font_montserrat_10, 0);
    lv_obj_align(hud_rec_lbl, LV_ALIGN_LEFT_MID, 72, 0);

    /* Dropdown toggle button */
    btn_dropdown = lv_btn_create(hud_bar);
    lv_obj_set_size(btn_dropdown, 120, 22);
    lv_obj_align(btn_dropdown, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(btn_dropdown, lv_color_hex(0x1a2030), 0);
    lv_obj_set_style_radius(btn_dropdown, 4, 0);
    lv_obj_add_event_cb(btn_dropdown, dropdown_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dd_lbl = lv_label_create(btn_dropdown);
    lv_label_set_text(dd_lbl, LV_SYMBOL_DOWN " SENSORDATA");
    lv_obj_set_style_text_color(dd_lbl, C_DIM, 0);
    lv_obj_set_style_text_font(dd_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(dd_lbl);

    hud_temp_lbl = lv_label_create(hud_bar);
    lv_label_set_text(hud_temp_lbl, "--.- C");
    lv_obj_set_style_text_color(hud_temp_lbl, C_DIM, 0);
    lv_obj_set_style_text_font(hud_temp_lbl, &lv_font_montserrat_10, 0);
    lv_obj_align(hud_temp_lbl, LV_ALIGN_RIGHT_MID, -10, 0);

    /* ── Dropdown panel (hidden above camera by default) ── */
    dropdown_panel = lv_obj_create(cam_area);
    lv_obj_set_pos(dropdown_panel, 0, -200);   /* hidden */
    lv_obj_set_size(dropdown_panel, CAM_W, 110);
    lv_obj_set_style_bg_color(dropdown_panel, lv_color_hex(0x080c18), 0);
    lv_obj_set_style_bg_opa(dropdown_panel, LV_OPA_90 + 10, 0);
    lv_obj_set_style_radius(dropdown_panel, 0, 0);
    lv_obj_set_style_border_side(dropdown_panel, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(dropdown_panel, C_ACCENT, 0);
    lv_obj_set_style_border_width(dropdown_panel, 1, 0);
    lv_obj_set_style_pad_all(dropdown_panel, 8, 0);
    lv_obj_clear_flag(dropdown_panel, LV_OBJ_FLAG_SCROLLABLE);

    /* dropdown grid: 3 columns */
    lv_obj_t *dd_c1 = make_card(dropdown_panel, 0,   0, 200, 90, LV_SYMBOL_HOME " MILJO");
    lv_obj_t *dd_c2 = make_card(dropdown_panel, 206, 0, 200, 90, LV_SYMBOL_WARNING " GAS");
    lv_obj_t *dd_c3 = make_card(dropdown_panel, 412, 0, 200, 90, "PARTIKLAR / LJUS");

    dd_temp  = val_label(dd_c1, 0, 16);
    dd_hum   = val_label(dd_c1, 0, 34);
    dd_press = val_label(dd_c1, 0, 52);
    dd_co    = val_label(dd_c2, 0, 16);
    dd_co2   = val_label(dd_c2, 0, 34);
    dd_tvoc  = val_label(dd_c2, 0, 52);
    dd_pm25  = val_label(dd_c3, 0, 16);
    dd_pm10  = val_label(dd_c3, 0, 34);
    dd_lux   = val_label(dd_c3, 0, 52);

    /* ── Bottom bar 640×120 ── */
    lv_obj_t *bbar = lv_obj_create(parent);
    lv_obj_set_pos(bbar, 0, CAM_H);
    lv_obj_set_size(bbar, BBAR_W, BBAR_H);
    lv_obj_set_style_bg_color(bbar, C_PANEL, 0);
    lv_obj_set_style_bg_opa(bbar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bbar, 0, 0);
    lv_obj_set_style_border_side(bbar, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(bbar, C_BORDER, 0);
    lv_obj_set_style_border_width(bbar, 1, 0);
    lv_obj_set_style_pad_all(bbar, 10, 0);
    lv_obj_clear_flag(bbar, LV_OBJ_FLAG_SCROLLABLE);

    /* Sensor chips */
    const char *chip_names[] = { "CO", "CO2", "PM2.5", "DIST", "LUX" };
    lv_obj_t **chip_vals[]   = { &chip_co_val, &chip_co2_val, &chip_pm25_val, &chip_dist_val, &chip_lux_val };
    lv_obj_t **chip_boxes[]  = { &chip_co_box, &chip_co2_box, &chip_pm25_box, &chip_dist_box, NULL };

    for (int i = 0; i < 5; i++) {
        lv_obj_t *chip = lv_obj_create(bbar);
        lv_obj_set_pos(chip, i * 80, 0);
        lv_obj_set_size(chip, 74, 90);
        lv_obj_set_style_bg_color(chip, C_CARD, 0);
        lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(chip, 6, 0);
        lv_obj_set_style_border_width(chip, 1, 0);
        lv_obj_set_style_border_color(chip, C_BORDER, 0);
        lv_obj_set_style_pad_all(chip, 6, 0);
        lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE);

        if (i < 4 && chip_boxes[i]) *chip_boxes[i] = chip;

        lv_obj_t *nl = lv_label_create(chip);
        lv_label_set_text(nl, chip_names[i]);
        lv_obj_set_style_text_color(nl, C_DIM, 0);
        lv_obj_set_style_text_font(nl, &lv_font_montserrat_10, 0);
        lv_obj_align(nl, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t *vl = lv_label_create(chip);
        lv_label_set_text(vl, "--");
        lv_obj_set_style_text_color(vl, C_TEXT, 0);
        lv_obj_set_style_text_font(vl, &lv_font_montserrat_16, 0);
        lv_obj_align(vl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        *chip_vals[i] = vl;
    }

    /* Steer / Throttle readout */
    lv_obj_t *readout = lv_obj_create(bbar);
    lv_obj_set_pos(readout, 420, 10);
    lv_obj_set_size(readout, 190, 72);
    lv_obj_set_style_bg_color(readout, C_CARD, 0);
    lv_obj_set_style_bg_opa(readout, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(readout, 6, 0);
    lv_obj_set_style_border_width(readout, 1, 0);
    lv_obj_set_style_border_color(readout, C_BORDER, 0);
    lv_obj_set_style_pad_all(readout, 8, 0);
    lv_obj_clear_flag(readout, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sl = lv_label_create(readout);
    lv_label_set_text(sl, "STYR");
    lv_obj_set_style_text_color(sl, C_DIM, 0);
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(sl, 0, 0);

    lbl_steer = lv_label_create(readout);
    lv_label_set_text(lbl_steer, "0%");
    lv_obj_set_style_text_color(lbl_steer, C_TEXT, 0);
    lv_obj_set_style_text_font(lbl_steer, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(lbl_steer, 0, 22);

    lv_obj_t *tl = lv_label_create(readout);
    lv_label_set_text(tl, "GAS");
    lv_obj_set_style_text_color(tl, C_DIM, 0);
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(tl, 100, 0);

    lbl_throttle = lv_label_create(readout);
    lv_label_set_text(lbl_throttle, "0%");
    lv_obj_set_style_text_color(lbl_throttle, C_TEXT, 0);
    lv_obj_set_style_text_font(lbl_throttle, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(lbl_throttle, 100, 22);

    /* ════════════════════════════════════════
     *  RIGHT PANEL (384 × 600)
     * ════════════════════════════════════════ */
    lv_obj_t *rp = lv_obj_create(parent);
    lv_obj_set_pos(rp, CAM_W, 0);
    lv_obj_set_size(rp, RPANEL_W, RPANEL_H);
    lv_obj_set_style_bg_color(rp, C_PANEL, 0);
    lv_obj_set_style_bg_opa(rp, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(rp, 0, 0);
    lv_obj_set_style_border_width(rp, 0, 0);
    lv_obj_set_style_pad_all(rp, 8, 0);
    lv_obj_clear_flag(rp, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Header ── */
    lv_obj_t *rp_hdr = lv_label_create(rp);
    lv_label_set_text(rp_hdr, "TELEMETRI");
    lv_obj_set_style_text_color(rp_hdr, C_ACCENT, 0);
    lv_obj_set_style_text_font(rp_hdr, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(rp_hdr, 0, 0);

    rp_time_lbl = lv_label_create(rp);
    lv_label_set_text(rp_time_lbl, "0s");
    lv_obj_set_style_text_color(rp_time_lbl, C_DIM, 0);
    lv_obj_set_style_text_font(rp_time_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(rp_time_lbl, 280, 0);

    rp_bat_lbl = lv_label_create(rp);
    lv_label_set_text(rp_bat_lbl, "100%");
    lv_obj_set_style_text_color(rp_bat_lbl, C_OK, 0);
    lv_obj_set_style_text_font(rp_bat_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(rp_bat_lbl, 320, 0);

    /* card dimensions */
    lv_coord_t cw = 176, ch1 = 78, ch2 = 100;
    lv_coord_t col2x = cw + 6;
    lv_coord_t row0 = 20;

    /* ── ENV card ── */
    lv_obj_t *c_env = make_card(rp, 0, row0, cw, ch1, LV_SYMBOL_HOME " MILJO");
    rp_temp  = val_label(c_env, 0, 16);
    rp_hum   = val_label(c_env, 0, 32);
    rp_press = val_label(c_env, 0, 48);

    /* ── GAS card ── */
    lv_obj_t *c_gas = make_card(rp, col2x, row0, cw, ch1, LV_SYMBOL_WARNING " GAS");
    rp_co   = val_label(c_gas, 0, 16);
    rp_co2  = val_label(c_gas, 0, 32);
    rp_tvoc = val_label(c_gas, 0, 48);

    lv_coord_t row1 = row0 + ch1 + 5;

    /* ── THERMAL card ── */
    lv_obj_t *c_therm = make_card(rp, 0, row1, cw, ch2, "TERMISK");
    rp_thermal_canvas = lv_canvas_create(c_therm);
    lv_canvas_set_buffer(rp_thermal_canvas, thermal_buf, 8, 8, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(rp_thermal_canvas, 80, 64);
    lv_obj_set_pos(rp_thermal_canvas, 0, 18);

    rp_tmin_lbl = lv_label_create(c_therm);
    lv_label_set_text(rp_tmin_lbl, "--");
    lv_obj_set_style_text_color(rp_tmin_lbl, C_DIM, 0);
    lv_obj_set_style_text_font(rp_tmin_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(rp_tmin_lbl, 90, 18);

    rp_tmax_lbl = lv_label_create(c_therm);
    lv_label_set_text(rp_tmax_lbl, "--");
    lv_obj_set_style_text_color(rp_tmax_lbl, C_TEXT, 0);
    lv_obj_set_style_text_font(rp_tmax_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(rp_tmax_lbl, 90, 36);

    /* ── IMU card ── */
    lv_obj_t *c_imu = make_card(rp, col2x, row1, cw, ch2, LV_SYMBOL_REFRESH " IMU");
    rp_roll  = val_label(c_imu, 0, 16);
    rp_pitch = val_label(c_imu, 0, 32);

    /* Compass canvas */
    rp_compass = lv_canvas_create(c_imu);
    lv_canvas_set_buffer(rp_compass, compass_buf, 52, 52, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(rp_compass, 52, 52);
    lv_obj_set_pos(rp_compass, 110, 16);

    lv_coord_t row2 = row1 + ch2 + 5;

    /* ── PARTICLES card ── */
    lv_obj_t *c_part = make_card(rp, 0, row2, cw, 56, "PARTIKLAR");
    rp_pm25 = val_label(c_part, 0, 16);
    rp_pm10 = val_label(c_part, 0, 32);

    /* ── RANGE card ── */
    lv_obj_t *c_rng = make_card(rp, col2x, row2, cw, 56, LV_SYMBOL_RIGHT " RANGE");
    rp_dist  = val_label(c_rng, 0, 16);
    rp_lux_r = val_label(c_rng, 0, 32);

    lv_coord_t row3 = row2 + 62;

    /* ── Alert bar ── */
    rp_alert_bar = lv_obj_create(rp);
    lv_obj_set_pos(rp_alert_bar, 0, row3);
    lv_obj_set_size(rp_alert_bar, RPANEL_W - 16, 26);
    lv_obj_set_style_bg_color(rp_alert_bar, C_OK, 0);
    lv_obj_set_style_bg_opa(rp_alert_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(rp_alert_bar, 5, 0);
    lv_obj_set_style_border_width(rp_alert_bar, 0, 0);
    lv_obj_set_style_pad_hor(rp_alert_bar, 10, 0);
    lv_obj_clear_flag(rp_alert_bar, LV_OBJ_FLAG_SCROLLABLE);

    rp_alert_lbl = lv_label_create(rp_alert_bar);
    lv_label_set_text(rp_alert_lbl, "STATUS: OK");
    lv_obj_set_style_text_color(rp_alert_lbl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(rp_alert_lbl, &lv_font_montserrat_10, 0);
    lv_obj_align(rp_alert_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    rp_alert_ev = lv_label_create(rp_alert_bar);
    lv_label_set_text(rp_alert_ev, "");
    lv_obj_set_style_text_color(rp_alert_ev, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(rp_alert_ev, &lv_font_montserrat_10, 0);
    lv_obj_align(rp_alert_ev, LV_ALIGN_RIGHT_MID, 0, 0);

    /* ── Bottom section: buttons + joystick ── */
    lv_coord_t btn_y = row3 + 34;
    lv_coord_t btn_w = 100, btn_h = 36;

    /* Lights button */
    btn_lights = lv_btn_create(rp);
    lv_obj_set_pos(btn_lights, 0, btn_y);
    lv_obj_set_size(btn_lights, btn_w, btn_h);
    lv_obj_set_style_bg_color(btn_lights, C_CARD, 0);
    lv_obj_set_style_radius(btn_lights, 6, 0);
    lv_obj_set_style_border_width(btn_lights, 1, 0);
    lv_obj_set_style_border_color(btn_lights, C_BORDER, 0);
    lv_obj_add_event_cb(btn_lights, lights_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ll = lv_label_create(btn_lights);
    lv_label_set_text(ll, LV_SYMBOL_EYE_OPEN " LAMPOR");
    lv_obj_set_style_text_color(ll, C_DIM, 0);
    lv_obj_set_style_text_font(ll, &lv_font_montserrat_10, 0);
    lv_obj_center(ll);

    /* Record button */
    btn_rec = lv_btn_create(rp);
    lv_obj_set_pos(btn_rec, 0, btn_y + btn_h + 6);
    lv_obj_set_size(btn_rec, btn_w, btn_h);
    lv_obj_set_style_bg_color(btn_rec, lv_color_hex(0x2a1010), 0);
    lv_obj_set_style_radius(btn_rec, 6, 0);
    lv_obj_set_style_border_width(btn_rec, 1, 0);
    lv_obj_set_style_border_color(btn_rec, C_DANGER, 0);
    lv_obj_add_event_cb(btn_rec, rec_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(btn_rec);
    lv_label_set_text(rl, LV_SYMBOL_STOP " SPELA IN");
    lv_obj_set_style_text_color(rl, C_DANGER, 0);
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_10, 0);
    lv_obj_center(rl);

    /* Snapshot button */
    btn_snap = lv_btn_create(rp);
    lv_obj_set_pos(btn_snap, 0, btn_y + 2 * (btn_h + 6));
    lv_obj_set_size(btn_snap, btn_w, btn_h);
    lv_obj_set_style_bg_color(btn_snap, C_CARD, 0);
    lv_obj_set_style_radius(btn_snap, 6, 0);
    lv_obj_set_style_border_width(btn_snap, 1, 0);
    lv_obj_set_style_border_color(btn_snap, C_BORDER, 0);
    lv_obj_t *snl = lv_label_create(btn_snap);
    lv_label_set_text(snl, LV_SYMBOL_IMAGE " BILD");
    lv_obj_set_style_text_color(snl, C_DIM, 0);
    lv_obj_set_style_text_font(snl, &lv_font_montserrat_10, 0);
    lv_obj_center(snl);

    /* ── Joystick visual (ring + knob) ── */
    lv_coord_t joy_sz = 140;
    lv_coord_t joy_x  = RPANEL_W - joy_sz - 20;
    lv_coord_t joy_y  = btn_y + 10;

    joy_ring = lv_obj_create(rp);
    lv_obj_set_pos(joy_ring, joy_x, joy_y);
    lv_obj_set_size(joy_ring, joy_sz, joy_sz);
    lv_obj_set_style_radius(joy_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(joy_ring, lv_color_hex(0x0c1220), 0);
    lv_obj_set_style_bg_opa(joy_ring, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(joy_ring, 2, 0);
    lv_obj_set_style_border_color(joy_ring, lv_color_hex(0x1a2a40), 0);
    lv_obj_set_style_pad_all(joy_ring, 0, 0);  /* Remove padding for precise coordinates */
    lv_obj_clear_flag(joy_ring, LV_OBJ_FLAG_SCROLLABLE);

    /* crosshair lines in joystick */
    static lv_point_t jh[] = {{10, 70}, {130, 70}};
    static lv_point_t jv[] = {{70, 10}, {70, 130}};
    lv_obj_t *jlh = lv_line_create(joy_ring);
    lv_line_set_points(jlh, jh, 2);
    lv_obj_set_style_line_color(jlh, C_ACCENT, 0);
    lv_obj_set_style_line_opa(jlh, LV_OPA_10, 0);
    lv_obj_set_style_line_width(jlh, 1, 0);
    lv_obj_t *jlv = lv_line_create(joy_ring);
    lv_line_set_points(jlv, jv, 2);
    lv_obj_set_style_line_color(jlv, C_ACCENT, 0);
    lv_obj_set_style_line_opa(jlv, LV_OPA_10, 0);
    lv_obj_set_style_line_width(jlv, 1, 0);

    /* direction labels */
    const char *dirs[] = { "FWD", "REV", "L", "R" };
    lv_coord_t dx[] = { 56, 56, 2, 118 };
    lv_coord_t dy[] = { 2, 124, 62, 62 };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *dl = lv_label_create(joy_ring);
        lv_label_set_text(dl, dirs[i]);
        lv_obj_set_style_text_color(dl, C_ACCENT, 0);
        lv_obj_set_style_text_opa(dl, LV_OPA_40, 0);
        lv_obj_set_style_text_font(dl, &lv_font_montserrat_10, 0);
        lv_obj_set_pos(dl, dx[i], dy[i]);
    }

    /* Knob (centered by default, moved by update) */
    joy_knob = lv_obj_create(joy_ring);
    lv_obj_set_size(joy_knob, 44, 44);
    lv_obj_set_style_radius(joy_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(joy_knob, lv_color_hex(0x2e5577), 0);
    lv_obj_set_style_bg_opa(joy_knob, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(joy_knob, 2, 0);
    lv_obj_set_style_border_color(joy_knob, C_ACCENT, 0);
    lv_obj_set_style_border_opa(joy_knob, LV_OPA_50, 0);
    lv_obj_clear_flag(joy_knob, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(joy_knob);
}

/* ═══════════════════════════════════════════
 *  TOGGLE DROPDOWN
 * ═══════════════════════════════════════════ */
void ui_dashboard_toggle_dropdown(void)
{
    dropdown_open = !dropdown_open;
    if (dropdown_open) {
        lv_obj_set_pos(dropdown_panel, 0, 30);  /* slide below HUD */
    } else {
        lv_obj_set_pos(dropdown_panel, 0, -200); /* hide above */
    }
}

/* ═══════════════════════════════════════════
 *  DRAW COMPASS  (52×52 canvas)
 * ═══════════════════════════════════════════ */
static void draw_compass(float yaw_deg)
{
    /* Clear */
    lv_canvas_fill_bg(rp_compass, lv_color_hex(0x0f1420), LV_OPA_COVER);

    /* Ring */
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = lv_color_hex(0x1a2a40);
    arc_dsc.width = 2;
    lv_canvas_draw_arc(rp_compass, 26, 26, 24, 0, 360, &arc_dsc);

    /* Needle */
    float rad = yaw_deg * 3.14159f / 180.0f;
    lv_coord_t nx = 26 + (lv_coord_t)(sinf(rad) * 18);
    lv_coord_t ny = 26 - (lv_coord_t)(cosf(rad) * 18);

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = C_DANGER;
    line_dsc.width = 2;
    lv_point_t pts[2] = { {26, 26}, {nx, ny} };
    lv_canvas_draw_line(rp_compass, pts, 2, &line_dsc);

    /* Center dot */
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = C_TEXT;
    rect_dsc.bg_opa   = LV_OPA_COVER;
    rect_dsc.radius   = LV_RADIUS_CIRCLE;
    lv_canvas_draw_rect(rp_compass, 24, 24, 4, 4, &rect_dsc);
}

/* ═══════════════════════════════════════════
 *  UPDATE
 * ═══════════════════════════════════════════ */
void ui_dashboard_update(const scout_sensors_t *d,
                         alert_level_t alert,
                         const joystick_input_t *joy)
{
    char buf[64];
    lv_color_t acol = alert_to_color(alert);

    /* ── HUD ── */
    lv_obj_set_style_bg_color(hud_alert_dot, acol, 0);
    snprintf(buf, sizeof(buf), "%.1f C", d->env.temperature_c);
    lv_label_set_text(hud_temp_lbl, buf);

    /* ── Dropdown ── */
    set_val(dd_temp,  "Temp  %.1f C",   d->env.temperature_c, 35, 50, true);
    set_val(dd_hum,   "Fukt  %.0f %%",  d->env.humidity_pct,  80, 0,  true);
    set_val(dd_press, "Tryck %.0f hPa", d->env.pressure_hpa,  0,  0,  true);
    set_val(dd_co,    "CO   %.1f ppm",  d->gas.co_ppm,        9,  35, true);
    set_val(dd_co2,   "CO2  %.0f ppm",  d->gas.co2_ppm,       1000, 2000, true);
    set_val(dd_tvoc,  "VOC  %.0f ppb",  d->gas.tvoc_ppb,      0,  0,  true);
    set_val(dd_pm25,  "PM2.5 %.0f ug",  d->particle.pm25,     55, 150, true);
    set_val(dd_pm10,  "PM10  %.0f ug",  d->particle.pm10,     0,  0,   true);
    snprintf(buf, sizeof(buf), "Ljus  %.0f lx", d->light.lux);
    lv_label_set_text(dd_lux, buf);

    /* ── Bottom chips ── */
    set_val(chip_co_val,   "%.1f",  d->gas.co_ppm,     9, 35, true);
    set_val(chip_co2_val,  "%.0f",  d->gas.co2_ppm,    1000, 2000, true);
    set_val(chip_pm25_val, "%.0f",  d->particle.pm25,   55, 150, true);
    set_val(chip_dist_val, "%.0f",  d->distance.distance_cm, 0, 0, true);
    snprintf(buf, sizeof(buf), "%.0f", d->light.lux);
    lv_label_set_text(chip_lux_val, buf);

    /* Chip border colours */
    lv_obj_set_style_border_color(chip_co_box,
        d->gas.co_ppm > 35 ? C_DANGER : d->gas.co_ppm > 9 ? C_WARN : C_BORDER, 0);
    lv_obj_set_style_border_color(chip_co2_box,
        d->gas.co2_ppm > 2000 ? C_DANGER : d->gas.co2_ppm > 1000 ? C_WARN : C_BORDER, 0);
    lv_obj_set_style_border_color(chip_pm25_box,
        d->particle.pm25 > 150 ? C_DANGER : d->particle.pm25 > 55 ? C_WARN : C_BORDER, 0);
    lv_obj_set_style_border_color(chip_dist_box,
        d->distance.object_detected ? C_WARN : C_BORDER, 0);

    /* ── Steer / throttle readout ── */
    int steer    = (int)(joy->x * 100);
    int throttle = (int)(joy->y * 100);

    if (steer > 0)      snprintf(buf, sizeof(buf), "R %d%%", steer);
    else if (steer < 0) snprintf(buf, sizeof(buf), "L %d%%", -steer);
    else                 snprintf(buf, sizeof(buf), "0%%");
    lv_label_set_text(lbl_steer, buf);
    lv_obj_set_style_text_color(lbl_steer,
        abs(steer) > 50 ? C_WARN : C_TEXT, 0);

    if (throttle > 0)      snprintf(buf, sizeof(buf), "+%d%%", throttle);
    else if (throttle < 0) snprintf(buf, sizeof(buf), "%d%%", throttle);
    else                    snprintf(buf, sizeof(buf), "0%%");
    lv_label_set_text(lbl_throttle, buf);
    lv_obj_set_style_text_color(lbl_throttle,
        throttle > 0 ? C_OK : throttle < 0 ? C_DANGER : C_TEXT, 0);

    /* ── Right panel ── */
    snprintf(buf, sizeof(buf), "%ds", d->timestamp_ms / 1000);
    lv_label_set_text(rp_time_lbl, buf);

    snprintf(buf, sizeof(buf), "%.0f%%", d->battery_pct);
    lv_label_set_text(rp_bat_lbl, buf);
    lv_obj_set_style_text_color(rp_bat_lbl,
        d->battery_pct > 50 ? C_OK : d->battery_pct > 20 ? C_WARN : C_DANGER, 0);

    /* Env */
    set_val(rp_temp,  "%.1f C",   d->env.temperature_c, 35, 50, true);
    set_val(rp_hum,   "%.0f %%RH", d->env.humidity_pct, 80, 0,  true);
    snprintf(buf, sizeof(buf), "%.0f hPa", d->env.pressure_hpa);
    lv_label_set_text(rp_press, buf);

    /* Gas */
    set_val(rp_co,   "CO  %.1f ppm",  d->gas.co_ppm,   9, 35, true);
    set_val(rp_co2,  "CO2 %.0f ppm",  d->gas.co2_ppm,  1000, 2000, true);
    snprintf(buf, sizeof(buf), "VOC %.0f ppb", d->gas.tvoc_ppb);
    lv_label_set_text(rp_tvoc, buf);

    /* Thermal */
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            thermal_buf[r * 8 + c] = temp_to_color(
                d->thermal.pixels[r][c], d->thermal.min_temp, d->thermal.max_temp);
    lv_obj_invalidate(rp_thermal_canvas);

    snprintf(buf, sizeof(buf), "Min %.1f", d->thermal.min_temp);
    lv_label_set_text(rp_tmin_lbl, buf);
    snprintf(buf, sizeof(buf), "Max %.1f", d->thermal.max_temp);
    lv_label_set_text(rp_tmax_lbl, buf);
    lv_obj_set_style_text_color(rp_tmax_lbl,
        d->thermal.max_temp > 60 ? C_DANGER : C_TEXT, 0);

    /* IMU */
    snprintf(buf, sizeof(buf), "Roll  %.1f", d->imu.roll_deg);
    lv_label_set_text(rp_roll, buf);
    snprintf(buf, sizeof(buf), "Pitch %.1f", d->imu.pitch_deg);
    lv_label_set_text(rp_pitch, buf);
    draw_compass(d->imu.yaw_deg);

    /* Particles */
    set_val(rp_pm25, "PM2.5 %.0f ug/m3", d->particle.pm25, 55, 150, true);
    snprintf(buf, sizeof(buf), "PM10  %.0f ug/m3", d->particle.pm10);
    lv_label_set_text(rp_pm10, buf);

    /* Range */
    set_val(rp_dist, "Dist %.0f cm", d->distance.distance_cm, 0, 0, true);
    lv_obj_set_style_text_color(rp_dist,
        d->distance.object_detected ? C_WARN : C_TEXT, 0);
    snprintf(buf, sizeof(buf), "Ljus %.0f lx", d->light.lux);
    lv_label_set_text(rp_lux_r, buf);

    /* Alert bar */
    lv_obj_set_style_bg_color(rp_alert_bar, acol, 0);
    snprintf(buf, sizeof(buf), "STATUS: %s", scout_alert_str(alert));
    lv_label_set_text(rp_alert_lbl, buf);

    /* ── Joystick knob position ── */
    lv_coord_t max_off = 48;
    lv_coord_t knob_cx = (lv_coord_t)(joy->x * max_off);
    lv_coord_t knob_cy = (lv_coord_t)(-joy->y * max_off);
    lv_obj_align(joy_knob, LV_ALIGN_CENTER, knob_cx, knob_cy);

    lv_obj_set_style_bg_color(joy_knob,
        (fabsf(joy->x) > 0.1f || fabsf(joy->y) > 0.1f)
            ? lv_color_hex(0x5ba4d9) : lv_color_hex(0x2e5577), 0);
    lv_obj_set_style_border_color(joy_ring,
        (fabsf(joy->x) > 0.1f || fabsf(joy->y) > 0.1f)
            ? lv_color_hex(0x3a6a90) : lv_color_hex(0x1a2a40), 0);
}
