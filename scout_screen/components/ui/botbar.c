#include "internal.h"
#include "botbar.h"

static lv_obj_t *s_theme_name_lbl;

void botbar_build(void)
{
    lv_obj_t *botbar = make_obj(s_root);
    lv_obj_add_style(botbar, &st_bar, 0);
    lv_obj_set_size(botbar, SCREEN_W - 2 * PANEL_GAP, BAR_H);
    lv_obj_align(botbar, LV_ALIGN_BOTTOM_MID, 0, -PANEL_GAP);

    const char *bot_keys[] = { "IP", "UDP" };
    const char *bot_vals[] = { S3_IP, XSTR(VID_PORT) };
    const int32_t key_w[]  = { 24, 38 };
    int32_t bot_x = 14;
    for(int i = 0; i < 2; i++) {
        lv_obj_t *k = make_label(botbar, bot_keys[i], &st_fg_lo, NULL);
        lv_obj_align(k, LV_ALIGN_LEFT_MID, bot_x, 0);
        bot_x += key_w[i];
        lv_obj_t *v = make_label(botbar, bot_vals[i], &st_fg_mid, NULL);
        lv_obj_align(v, LV_ALIGN_LEFT_MID, bot_x, 0);
        bot_x += (i == 0) ? 110 : 50;
    }
    make_vsep(botbar, LV_ALIGN_LEFT_MID, 140);
    make_vsep(botbar, LV_ALIGN_LEFT_MID, 246);

    lv_obj_t *swatch = make_obj(botbar);
    lv_obj_add_style(swatch, &st_fill_accent, 0);
    lv_obj_set_size(swatch, 8, 8);
    lv_obj_align(swatch, LV_ALIGN_LEFT_MID, 264, 0);
    lv_obj_set_style_radius(swatch, 2, 0);

    s_theme_name_lbl = make_label(botbar, s_th->name, &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(s_theme_name_lbl, 2, 0);
    lv_obj_align(s_theme_name_lbl, LV_ALIGN_LEFT_MID, 280, 0);

    make_vsep(botbar, LV_ALIGN_RIGHT_MID, -165);

    lv_obj_t *rtos_k = make_label(botbar, "RTOS", &st_fg_lo, NULL);
    lv_obj_align(rtos_k, LV_ALIGN_RIGHT_MID, -120, 0);
    lv_obj_t *rtos_v = make_label(botbar, "FREERTOS", &st_fg_accent, NULL);
    lv_obj_align(rtos_v, LV_ALIGN_RIGHT_MID, -14, 0);
}

void botbar_set_theme_name(const char *name)
{
    lv_label_set_text(s_theme_name_lbl, name);
}
