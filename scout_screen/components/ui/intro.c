#include "internal.h"
#include "intro.h"

#define INTRO_BAR_W   440
#define INTRO_BAR_H   10
#define INTRO_FILL_W  (INTRO_BAR_W - 6)
#define INTRO_FILL_H  (INTRO_BAR_H - 6)
#define INTRO_HOLD_MS 1200

#define INTRO_LOGO_Y    (-96)
#define INTRO_RULE_Y    (-26)
#define INTRO_SUB_Y     (-4)
#define INTRO_DOTS_Y    64
#define INTRO_BAR_Y     96
#define INTRO_TEXT_Y    122
#define INTRO_RULE_W    300
#define INTRO_FRAME     28
#define INTRO_CORNER    34
#define INTRO_TAG_DY    (INTRO_FRAME + INTRO_CORNER / 2 - 4)
#define INTRO_DOT_GAP   22
#define INTRO_MAX_STEPS 12

static lv_obj_t *s_intro_overlay;
static lv_obj_t *s_intro_bar_fill;
static lv_obj_t *s_intro_status;
static lv_obj_t *s_intro_pct;
static lv_obj_t *s_intro_dots[INTRO_MAX_STEPS];
static uint8_t   s_intro_total;
static uint8_t   s_intro_step;

static void intro_close_cb(lv_timer_t *t)
{
    (void)t;
    lv_obj_del(s_intro_overlay);
    s_intro_overlay = NULL;
}

static lv_obj_t *intro_frame_label(const char *text, lv_style_t *fg,
                                   lv_align_t align, int32_t x, int32_t y)
{
    lv_obj_t *l = make_label(s_intro_overlay, text, fg, NULL);
    lv_obj_set_style_text_letter_space(l, 2, 0);
    lv_obj_align(l, align, x, y);
    return l;
}

void scout_ui_intro_screen(uint8_t total_steps)
{
    s_intro_total = total_steps ? total_steps : 1;
    if(s_intro_total > INTRO_MAX_STEPS) s_intro_total = INTRO_MAX_STEPS;
    s_intro_step  = 0;

    s_intro_overlay = make_obj(lv_scr_act());
    lv_obj_add_style(s_intro_overlay, &st_fill_bg, 0);
    lv_obj_set_size(s_intro_overlay, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(s_intro_overlay, 0, 0);

    make_corner(s_intro_overlay, INTRO_FRAME, INTRO_FRAME,
                INTRO_CORNER, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_TOP, LV_OPA_50);
    make_corner(s_intro_overlay, SCREEN_W - INTRO_FRAME - INTRO_CORNER, INTRO_FRAME,
                INTRO_CORNER, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_TOP, LV_OPA_50);
    make_corner(s_intro_overlay, INTRO_FRAME, SCREEN_H - INTRO_FRAME - INTRO_CORNER,
                INTRO_CORNER, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, LV_OPA_50);
    make_corner(s_intro_overlay, SCREEN_W - INTRO_FRAME - INTRO_CORNER,
                SCREEN_H - INTRO_FRAME - INTRO_CORNER,
                INTRO_CORNER, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM, LV_OPA_50);

    intro_frame_label("LINTECH", &st_fg_lo,
                      LV_ALIGN_TOP_LEFT, INTRO_FRAME + 14, INTRO_TAG_DY);
    intro_frame_label("BOOT SEQUENCE", &st_fg_lo,
                      LV_ALIGN_TOP_RIGHT, -(INTRO_FRAME + 14), INTRO_TAG_DY);
    intro_frame_label("FW v1.0.0", &st_fg_lo,
                      LV_ALIGN_BOTTOM_LEFT, INTRO_FRAME + 14, -INTRO_TAG_DY);
    intro_frame_label("FREERTOS", &st_fg_accent,
                      LV_ALIGN_BOTTOM_RIGHT, -(INTRO_FRAME + 14), -INTRO_TAG_DY);
    intro_frame_label("RTOS", &st_fg_lo,
                      LV_ALIGN_BOTTOM_RIGHT, -(INTRO_FRAME + 106), -INTRO_TAG_DY);

    lv_obj_t *logo = make_label(s_intro_overlay, "SCOUT", &st_fg_accent, &st_font_logo);
    lv_obj_set_style_text_letter_space(logo, 8, 0);
    lv_obj_align(logo, LV_ALIGN_CENTER, 4, INTRO_LOGO_Y);

    lv_obj_t *rule_l = make_obj(s_intro_overlay);
    lv_obj_set_size(rule_l, INTRO_RULE_W / 2, 2);
    lv_obj_align(rule_l, LV_ALIGN_CENTER, -INTRO_RULE_W / 4, INTRO_RULE_Y);
    lv_obj_set_style_bg_color(rule_l, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_grad_color(rule_l, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_grad_dir(rule_l, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(rule_l, LV_OPA_COVER, 0);

    lv_obj_t *rule_r = make_obj(s_intro_overlay);
    lv_obj_set_size(rule_r, INTRO_RULE_W / 2, 2);
    lv_obj_align(rule_r, LV_ALIGN_CENTER, INTRO_RULE_W / 4, INTRO_RULE_Y);
    lv_obj_set_style_bg_color(rule_r, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_grad_color(rule_r, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_grad_dir(rule_r, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(rule_r, LV_OPA_COVER, 0);

    lv_obj_t *sub = make_label(s_intro_overlay, "DEVELOPED BY LINTECH", &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(sub, 8, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, INTRO_SUB_Y);

    int32_t dots_x0 = -(int32_t)(s_intro_total - 1) * INTRO_DOT_GAP / 2;
    for(int i = 0; i < s_intro_total; i++) {
        lv_obj_t *d = make_obj(s_intro_overlay);
        lv_obj_add_style(d, &st_fill_line, 0);
        lv_obj_set_size(d, 6, 6);
        lv_obj_align(d, LV_ALIGN_CENTER, dots_x0 + i * INTRO_DOT_GAP, INTRO_DOTS_Y);
        lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
        s_intro_dots[i] = d;
    }

    lv_obj_t *track = make_obj(s_intro_overlay);
    lv_obj_add_style(track, &st_card, 0);
    lv_obj_set_size(track, INTRO_BAR_W, INTRO_BAR_H);
    lv_obj_align(track, LV_ALIGN_CENTER, 0, INTRO_BAR_Y);
    lv_obj_set_style_bg_color(track, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_radius(track, INTRO_BAR_H / 2, 0);
    lv_obj_set_style_pad_all(track, 2, 0);

    s_intro_bar_fill = make_obj(track);
    lv_obj_set_size(s_intro_bar_fill, 0, INTRO_FILL_H);
    lv_obj_set_pos(s_intro_bar_fill, 0, 0);
    lv_obj_set_style_radius(s_intro_bar_fill, INTRO_FILL_H / 2, 0);
    lv_obj_set_style_bg_color(s_intro_bar_fill, lv_color_hex(COL_ACCENT_DEEP), 0);
    lv_obj_set_style_bg_grad_color(s_intro_bar_fill, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_grad_dir(s_intro_bar_fill, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(s_intro_bar_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(s_intro_bar_fill, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_shadow_width(s_intro_bar_fill, 12, 0);
    lv_obj_set_style_shadow_spread(s_intro_bar_fill, 0, 0);
    lv_obj_set_style_shadow_opa(s_intro_bar_fill, LV_OPA_40, 0);

    lv_obj_t *sdot = make_obj(s_intro_overlay);
    lv_obj_add_style(sdot, &st_fill_accent, 0);
    lv_obj_set_size(sdot, 6, 6);
    lv_obj_align(sdot, LV_ALIGN_CENTER, -(INTRO_BAR_W / 2) + 3, INTRO_TEXT_Y);
    lv_obj_set_style_radius(sdot, LV_RADIUS_CIRCLE, 0);

    s_intro_status = make_label(s_intro_overlay, "STARTING", &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(s_intro_status, 4, 0);
    lv_obj_set_width(s_intro_status, 240);
    lv_obj_set_style_text_align(s_intro_status, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(s_intro_status, LV_ALIGN_CENTER, -(INTRO_BAR_W / 2) + 136, INTRO_TEXT_Y);

    s_intro_pct = make_label(s_intro_overlay, "0%", &st_fg_hi, NULL);
    lv_obj_set_style_text_letter_space(s_intro_pct, 2, 0);
    lv_obj_set_width(s_intro_pct, 80);
    lv_obj_set_style_text_align(s_intro_pct, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(s_intro_pct, LV_ALIGN_CENTER, (INTRO_BAR_W / 2) - 40, INTRO_TEXT_Y);
}

void scout_ui_intro_step(const char *label)
{
    if(s_intro_overlay == NULL) return;

    if(s_intro_step < s_intro_total) s_intro_step++;
    lv_label_set_text(s_intro_status, label);
    lv_label_set_text_fmt(s_intro_pct, "%d%%", 100 * s_intro_step / s_intro_total);
    lv_obj_set_width(s_intro_bar_fill, INTRO_FILL_W * s_intro_step / s_intro_total);

    if(s_intro_step >= 1 && s_intro_step <= s_intro_total &&
       s_intro_dots[s_intro_step - 1]) {
        lv_obj_set_style_bg_color(s_intro_dots[s_intro_step - 1],
                                  lv_color_hex(COL_ACCENT), 0);
    }

    if(s_intro_step == s_intro_total) {
        lv_timer_t *t = lv_timer_create(intro_close_cb, INTRO_HOLD_MS, NULL);
        lv_timer_set_repeat_count(t, 1);
    }

    // Render directly — during boot the render loop is not running yet.
    lv_refr_now(lv_disp_get_default());
}
