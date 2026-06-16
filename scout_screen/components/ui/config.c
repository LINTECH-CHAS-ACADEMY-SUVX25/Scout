#include "internal.h"
#include "config.h"

#define CFG_PANEL_H CAM_H

static lv_obj_t     *s_cfg_panel;
static lv_obj_t     *s_cam_sw;
static lv_obj_t     *s_quality_sl;
static lv_obj_t     *s_ae_lvl_sl;
static lv_obj_t     *s_gain_sl;
static volatile bool s_cfg_dirty;
static bool          s_config_open;

static const char *const s_quality_labels[] = {"LOW", "MEDIUM", "HIGH", "ULTRA"};

static void quality_label_event(lv_event_t *e)
{
    lv_obj_t *lbl = lv_event_get_user_data(e);
    int v = (int)lv_slider_get_value(lv_event_get_target(e));
    lv_label_set_text(lbl, s_quality_labels[v]);
    s_cfg_dirty = true;
}

static void autoval_label_event(lv_event_t *e)
{
    lv_obj_t *lbl = lv_event_get_user_data(e);
    int v = (int)lv_slider_get_value(lv_event_get_target(e));
    if(v == 0) lv_label_set_text(lbl, "AUTO");
    else       lv_label_set_text_fmt(lbl, "%d", v);
    s_cfg_dirty = true;
}

static void cfg_sw_event(lv_event_t *e) { (void)e; s_cfg_dirty = true; }

static lv_obj_t *make_slider_row(lv_obj_t *panel, const char *key, const char *init_val,
                                  int32_t min, int32_t max, int32_t def,
                                  int32_t y, lv_event_cb_t val_cb)
{
    lv_obj_t *k = make_label(panel, key, &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(k, 2, 0);
    lv_obj_set_pos(k, PAD, y);

    lv_obj_t *v = make_label(panel, init_val, &st_fg_accent, NULL);
    lv_obj_set_style_text_letter_space(v, 2, 0);
    lv_obj_set_width(v, 64);
    lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(v, PAD + ROW_W - 64, y);

    lv_obj_t *sl = lv_slider_create(panel);
    int32_t sl_w = ROW_W * 9 / 10;
    int32_t sl_x = (SIDE_W - sl_w) / 2;
    lv_obj_set_size(sl, sl_w, 18);
    lv_obj_set_pos(sl, sl_x, y + 15);
    lv_slider_set_range(sl, min, max);
    lv_slider_set_value(sl, def, LV_ANIM_OFF);

    lv_obj_add_style(sl, &st_slider_main, LV_PART_MAIN);
    lv_obj_add_style(sl, &st_slider_ind,  LV_PART_INDICATOR);
    lv_obj_add_style(sl, &st_slider_knob, LV_PART_KNOB);

    lv_obj_add_event_cb(sl, val_cb, LV_EVENT_VALUE_CHANGED, v);
    return sl;
}

static lv_obj_t *make_switch_row(lv_obj_t *panel, const char *key, int32_t y, bool on)
{
    lv_obj_t *k = make_label(panel, key, &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(k, 2, 0);
    lv_obj_set_pos(k, PAD, y);

    lv_obj_t *sw = lv_switch_create(panel);
    lv_obj_set_size(sw, 40, 20);
    lv_obj_set_pos(sw, PAD + ROW_W - 40, y - 6);
    lv_obj_add_style(sw, &st_slider_main, LV_PART_MAIN);
    lv_obj_add_style(sw, &st_slider_ind,  LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_style(sw, &st_slider_knob, LV_PART_KNOB);
    if(on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    return sw;
}

static void make_cfg_sep(lv_obj_t *panel, int32_t y)
{
    lv_obj_t *s = make_obj(panel);
    lv_obj_add_style(s, &st_fill_line, 0);
    lv_obj_set_size(s, ROW_W, 1);
    lv_obj_set_pos(s, PAD, y);
}

void config_panel_build(void)
{
    s_cfg_panel = make_panel(SCREEN_W - PANEL_GAP - SIDE_W, CAM_Y, SIDE_W, CFG_PANEL_H);
    lv_obj_add_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);

    make_section_hdr(s_cfg_panel, "CAM CONFIG", HDR_Y);

    s_cam_sw     = make_switch_row(s_cfg_panel, "CAMERA", 68, true);
    lv_obj_add_event_cb(s_cam_sw, cfg_sw_event, LV_EVENT_VALUE_CHANGED, NULL);
    make_cfg_sep(s_cfg_panel, 116);
    s_quality_sl = make_slider_row(s_cfg_panel, "QUALITY", "MEDIUM", 0,  3, 1, 148, quality_label_event);
    make_cfg_sep(s_cfg_panel, 196);
    s_ae_lvl_sl  = make_slider_row(s_cfg_panel, "EXPOS",   "AUTO",   0, 10, 0, 228, autoval_label_event);
    make_cfg_sep(s_cfg_panel, 276);
    s_gain_sl    = make_slider_row(s_cfg_panel, "GAIN",    "AUTO",   0, 10, 0, 308, autoval_label_event);

    if(s_config_open) lv_obj_clear_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);
}

void ui_config_toggle(void)
{
    s_config_open = !s_config_open;
    if(s_config_open) {
        lv_obj_clear_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_cfg_panel);
    } else {
        lv_obj_add_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_config_close(void)
{
    s_config_open = false;
    lv_obj_add_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);
}

bool scout_ui_cfg_dirty_take(void)
{
    bool d = s_cfg_dirty;
    s_cfg_dirty = false;
    return d;
}

static const int8_t s_quality_map[4] = {63, 30, 16, 4};

void scout_ui_get_cam_cfg(bool *cam_on, int8_t *quality,
                           int8_t *ae_level, int8_t *agc_gain)
{
    *cam_on   = lv_obj_has_state(s_cam_sw, LV_STATE_CHECKED);
    *quality  = s_quality_map[lv_slider_get_value(s_quality_sl)];
    *ae_level = (int8_t)lv_slider_get_value(s_ae_lvl_sl);
    *agc_gain = (int8_t)lv_slider_get_value(s_gain_sl);
}
