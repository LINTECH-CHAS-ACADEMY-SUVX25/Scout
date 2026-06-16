#include "internal.h"
#include "topbar.h"
#include "config.h"
#include "menu.h"

static lv_obj_t *s_wifi_dot;
static lv_obj_t *s_wifi_arcs[3];
static lv_obj_t *s_wifi_slash;
static lv_obj_t *s_link_dot;
static lv_obj_t *s_link_lbl;
static uint8_t   s_wifi_level;

static lv_obj_t *make_wifi_arc(lv_obj_t *parent, int32_t d, int32_t cx, int32_t cy)
{
    lv_obj_t *a = lv_arc_create(parent);
    lv_obj_set_size(a, d, d);
    lv_obj_set_pos(a, cx - d / 2, cy - d / 2);
    lv_arc_set_bg_angles(a, 225, 315);
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(a, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, lv_color_hex(COL_LINE), LV_PART_MAIN);
    lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return a;
}

static void config_event(lv_event_t *e) { (void)e; ui_menu_close();   ui_config_toggle(); }
static void themes_event(lv_event_t *e) { (void)e; ui_config_close(); ui_menu_toggle(); }

void topbar_build(void)
{
    lv_obj_t *topbar = make_obj(s_root);
    lv_obj_add_style(topbar, &st_bar, 0);
    lv_obj_set_size(topbar, SCREEN_W - 2 * PANEL_GAP, BAR_H);
    lv_obj_align(topbar, LV_ALIGN_TOP_MID, 0, PANEL_GAP);

    lv_obj_t *logo = make_label(topbar, "SCOUT", &st_fg_accent, NULL);
    lv_obj_set_style_text_letter_space(logo, 6, 0);
    lv_obj_align(logo, LV_ALIGN_LEFT_MID, 14, 0);

    make_vsep(topbar, LV_ALIGN_LEFT_MID, 100);

    lv_obj_t *tagline = make_label(topbar, "LINTECH", &st_fg_lo, NULL);
    lv_obj_set_style_text_letter_space(tagline, 2, 0);
    lv_obj_align(tagline, LV_ALIGN_LEFT_MID, 116, 0);

    s_link_dot = make_obj(topbar);
    lv_obj_add_style(s_link_dot, &st_fill_bad,  0);
    lv_obj_add_style(s_link_dot, &st_fill_good, LV_STATE_USER_1);
    lv_obj_set_size(s_link_dot, 6, 6);
    lv_obj_align(s_link_dot, LV_ALIGN_CENTER, -36, 0);
    lv_obj_set_style_radius(s_link_dot, LV_RADIUS_CIRCLE, 0);

    s_link_lbl = make_label(topbar, "NO LINK", &st_fg_lo, NULL);
    lv_obj_add_style(s_link_lbl, &st_fg_mid, LV_STATE_USER_1);
    lv_obj_set_style_text_letter_space(s_link_lbl, 2, 0);
    lv_obj_set_width(s_link_lbl, 80);
    lv_obj_set_style_text_align(s_link_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(s_link_lbl, LV_ALIGN_CENTER, 18, 0);

    lv_obj_t *cfg_lbl = make_label(topbar, "CONFIG", &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(cfg_lbl, 2, 0);
    lv_obj_align(cfg_lbl, LV_ALIGN_RIGHT_MID, -144, 0);
    lv_obj_add_flag(cfg_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(cfg_lbl, 10);
    lv_obj_add_event_cb(cfg_lbl, config_event, LV_EVENT_CLICKED, NULL);

    make_vsep(topbar, LV_ALIGN_RIGHT_MID, -130);

    lv_obj_t *themes = make_label(topbar, "THEMES", &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(themes, 2, 0);
    lv_obj_align(themes, LV_ALIGN_RIGHT_MID, -66, 0);
    lv_obj_add_flag(themes, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(themes, 10);
    lv_obj_add_event_cb(themes, themes_event, LV_EVENT_CLICKED, NULL);

    make_vsep(topbar, LV_ALIGN_RIGHT_MID, -52);

    lv_obj_t *wifi_icon = make_obj(topbar);
    lv_obj_set_size(wifi_icon, 28, 15);
    lv_obj_align(wifi_icon, LV_ALIGN_RIGHT_MID, -14, -2);

    s_wifi_dot = make_obj(wifi_icon);
    lv_obj_add_style(s_wifi_dot, &st_fill_bad,    0);
    lv_obj_add_style(s_wifi_dot, &st_fill_texthi, LV_STATE_USER_1);
    lv_obj_set_size(s_wifi_dot, 4, 4);
    lv_obj_set_pos(s_wifi_dot, 12, 11);
    lv_obj_set_style_radius(s_wifi_dot, LV_RADIUS_CIRCLE, 0);

    for(int i = 0; i < 3; i++)
        s_wifi_arcs[i] = make_wifi_arc(wifi_icon, 10 + 8 * i, 14, 13);

    static const lv_point_t slash_pts[2] = {{2, 14}, {26, 1}};
    s_wifi_slash = lv_line_create(wifi_icon);
    lv_line_set_points(s_wifi_slash, slash_pts, 2);
    lv_obj_set_style_line_color(s_wifi_slash, lv_color_hex(COL_BAD), 0);
    lv_obj_set_style_line_width(s_wifi_slash, 2, 0);
    lv_obj_add_flag(s_wifi_slash, LV_OBJ_FLAG_HIDDEN);
}

void scout_ui_update(uint8_t wifi_level)
{
    s_wifi_level = wifi_level;

    if(wifi_level == 0) {
        lv_obj_clear_flag(s_wifi_slash, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(s_wifi_dot,  LV_STATE_USER_1);
        lv_obj_clear_state(s_link_dot,  LV_STATE_USER_1);
        lv_obj_clear_state(s_link_lbl,  LV_STATE_USER_1);
    } else {
        lv_obj_add_flag(s_wifi_slash, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(s_wifi_dot,  LV_STATE_USER_1);
        lv_obj_add_state(s_link_dot,  LV_STATE_USER_1);
        lv_obj_add_state(s_link_lbl,  LV_STATE_USER_1);
    }
    lv_obj_set_style_line_color(s_wifi_slash, lv_color_hex(COL_BAD), 0);
    for(int i = 0; i < 3; i++) {
        lv_obj_set_style_arc_color(s_wifi_arcs[i],
            lv_color_hex(wifi_level > (uint8_t)(i + 1) ? COL_TEXT_HI : COL_LINE),
            LV_PART_MAIN);
    }
    lv_label_set_text(s_link_lbl, wifi_level ? "LIVE" : "NO LINK");
}

void topbar_refresh_theme(void)
{
    scout_ui_update(s_wifi_level);
}
