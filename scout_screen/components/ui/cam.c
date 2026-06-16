#include "internal.h"
#include "cam.h"

static lv_obj_t    *s_scene_overlay;
static lv_obj_t    *s_scene_label;
static const char  *s_overlay_text;

static void make_cam_tick(int32_t x, int32_t y, bool horiz)
{
    lv_obj_t *t = make_obj(s_root);
    lv_obj_add_style(t, &st_fill_accent, 0);
    lv_obj_set_size(t, horiz ? 14 : 2, horiz ? 2 : 14);
    lv_obj_set_pos(t, x, y);
    lv_obj_set_style_radius(t, 1, 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_70, 0);
}

static void make_scene_overlay(void)
{
    s_scene_overlay = make_obj(s_root);
    lv_obj_add_style(s_scene_overlay, &st_fill_bg, 0);
    lv_obj_set_size(s_scene_overlay, CAM_W, CAM_H);
    lv_obj_set_pos(s_scene_overlay, CAM_X, CAM_Y);

    s_scene_label = make_label(s_scene_overlay, "", &st_fg_hi, &st_font_scene);
    lv_obj_set_width(s_scene_label, CAM_W - 2 * PAD);
    lv_obj_set_style_text_align(s_scene_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(s_scene_label, 2, 0);
    lv_obj_set_style_text_line_space(s_scene_label, 8, 0);
    lv_obj_align(s_scene_label, LV_ALIGN_CENTER, 0, 0);

    scout_ui_overlay(s_overlay_text);
}

void cam_decorations_build(void)
{
    make_corner(s_root, CAM_X - CAM_GAP, CAM_Y - CAM_GAP,
                CAM_CORNER, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT, LV_OPA_COVER);
    make_corner(s_root, CAM_X + CAM_W + CAM_GAP - CAM_CORNER, CAM_Y - CAM_GAP,
                CAM_CORNER, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT, LV_OPA_COVER);
    make_corner(s_root, CAM_X - CAM_GAP, CAM_Y + CAM_H + CAM_GAP - CAM_CORNER,
                CAM_CORNER, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_LEFT, LV_OPA_COVER);
    make_corner(s_root, CAM_X + CAM_W + CAM_GAP - CAM_CORNER,
                CAM_Y + CAM_H + CAM_GAP - CAM_CORNER,
                CAM_CORNER, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT, LV_OPA_COVER);

    make_cam_tick(CAM_X + CAM_W / 2 - 7, CAM_Y - CAM_GAP + 1,         true);
    make_cam_tick(CAM_X + CAM_W / 2 - 7, CAM_Y + CAM_H + CAM_GAP - 3, true);
    make_cam_tick(CAM_X - CAM_GAP + 1,        CAM_Y + CAM_H / 2 - 7, false);
    make_cam_tick(CAM_X + CAM_W + CAM_GAP - 3, CAM_Y + CAM_H / 2 - 7, false);

    make_scene_overlay();
}

void scout_ui_overlay(const char *text)
{
    s_overlay_text = text;
    if(text) {
        lv_label_set_text(s_scene_label, text);
        lv_obj_clear_flag(s_scene_overlay, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_scene_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}
