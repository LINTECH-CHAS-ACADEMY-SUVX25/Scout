#include "internal.h"
#include "menu.h"
#include "scout_ui.h"

static lv_obj_t *s_theme_menu;
static lv_obj_t *s_theme_dots[THEME_COUNT];
static uint8_t   s_pending_theme;

static void theme_apply_cb(lv_timer_t *t)
{
    (void)t;
    scout_ui_set_theme(s_pending_theme);
}

static void theme_item_event(lv_event_t *e)
{
    s_pending_theme = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_add_flag(s_theme_menu, LV_OBJ_FLAG_HIDDEN);
    lv_timer_t *t = lv_timer_create(theme_apply_cb, 0, NULL);
    lv_timer_set_repeat_count(t, 1);
}

void theme_menu_build(void)
{
    s_theme_menu = make_obj(s_root);
    lv_obj_add_style(s_theme_menu, &st_card, 0);
    lv_obj_set_size(s_theme_menu, MENU_W, THEME_COUNT * MENU_ITEM_H + 2 * MENU_PAD);
    lv_obj_set_pos(s_theme_menu, SCREEN_W - PANEL_GAP - MENU_W - 6, PANEL_GAP + BAR_H + 4);
    lv_obj_add_flag(s_theme_menu, LV_OBJ_FLAG_HIDDEN);

    for(uint8_t i = 0; i < THEME_COUNT; i++) {
        lv_obj_t *item = lv_obj_create(s_theme_menu);
        lv_obj_add_style(item, &st_reset, 0);
        lv_obj_add_style(item, &st_menu_item, 0);
        lv_obj_add_style(item, &st_pressed, LV_STATE_PRESSED);
        lv_obj_set_size(item, MENU_W - 2 * MENU_PAD, MENU_ITEM_H);
        lv_obj_set_pos(item, MENU_PAD, MENU_PAD + i * MENU_ITEM_H);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(item, theme_item_event, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *l = make_label(item, s_themes[i].name, NULL, NULL);
        lv_obj_set_style_text_color(l, lv_color_hex(s_themes[i].accent), 0);
        lv_obj_set_style_text_letter_space(l, 2, 0);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 10, 0);

        lv_obj_t *dot = make_obj(item);
        lv_obj_set_size(dot, 4, 4);
        lv_obj_align(dot, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(s_themes[i].accent), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        if(&s_themes[i] != s_th) lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        s_theme_dots[i] = dot;
    }
}

void ui_menu_toggle(void)
{
    if(lv_obj_has_flag(s_theme_menu, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(s_theme_menu, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_theme_menu);
    } else {
        lv_obj_add_flag(s_theme_menu, LV_OBJ_FLAG_HIDDEN);
    }
}

void menu_set_active(uint8_t idx)
{
    for(uint8_t i = 0; i < THEME_COUNT; i++)
        lv_obj_add_flag(s_theme_dots[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_theme_dots[idx], LV_OBJ_FLAG_HIDDEN);
}
