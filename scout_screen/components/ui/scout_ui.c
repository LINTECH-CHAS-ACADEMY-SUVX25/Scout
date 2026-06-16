#include "scout_ui.h"
#include "internal.h"
#include "topbar.h"
#include "botbar.h"
#include "tele.h"
#include "joy.h"
#include "cam.h"
#include "config.h"
#include "menu.h"
#include "intro.h"

// Root container — all panels are children of this object so a single
// lv_obj_del(s_root) would tear down the whole UI if needed.
lv_obj_t *s_root;

// ---------------------------------------------------------------------------
// Shared widget helpers — used by every panel builder via scout_ui_internal.h
// ---------------------------------------------------------------------------

lv_obj_t *make_obj(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_add_style(o, &st_reset, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                     lv_style_t *fg, lv_style_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    if(fg) lv_obj_add_style(l, fg, 0);
    lv_obj_add_style(l, font ? font : &st_font_sm, 0);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
    return l;
}

void make_vsep(lv_obj_t *parent, lv_align_t align, int32_t x)
{
    lv_obj_t *s = make_obj(parent);
    lv_obj_add_style(s, &st_fill_line, 0);
    lv_obj_set_size(s, 1, 12);
    lv_obj_align(s, align, x, 0);
}

lv_obj_t *make_corner(lv_obj_t *parent, int32_t x, int32_t y,
                      int32_t size, lv_border_side_t side, lv_opa_t opa)
{
    lv_obj_t *c = make_obj(parent);
    lv_obj_add_style(c, &st_border_accent, 0);
    lv_obj_set_size(c, size, size);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_border_width(c, 2, 0);
    lv_obj_set_style_border_side(c, side, 0);
    lv_obj_set_style_border_opa(c, opa, 0);
    return c;
}

lv_obj_t *make_panel(int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *p = make_obj(s_root);
    lv_obj_add_style(p, &st_panel, 0);
    lv_obj_set_size(p, w, h);
    lv_obj_set_pos(p, x, y);
    return p;
}

void make_section_hdr(lv_obj_t *parent, const char *text, int32_t y)
{
    lv_obj_t *mark = make_obj(parent);
    lv_obj_add_style(mark, &st_fill_accent, 0);
    lv_obj_set_size(mark, 3, 14);
    lv_obj_set_pos(mark, PAD, y - 3);
    lv_obj_set_style_radius(mark, 1, 0);

    lv_obj_t *l = make_label(parent, text, &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(l, 2, 0);
    lv_obj_set_pos(l, PAD + 10, y);

    lv_obj_t *rule = make_obj(parent);
    lv_obj_add_style(rule, &st_rule, 0);
    lv_obj_set_size(rule, ROW_W, 1);
    lv_obj_set_pos(rule, PAD, y + 18);
}

// ---------------------------------------------------------------------------
// Assembly
// ---------------------------------------------------------------------------

static void build_ui(void)
{
    make_stripe_tile();
    styles_init();
    styles_apply_theme();

    lv_obj_add_style(lv_scr_act(), &st_fill_bg, 0);

    s_root = make_obj(lv_scr_act());
    lv_obj_set_size(s_root, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(s_root, 0, 0);

    topbar_build();
    botbar_build();
    tele_panel_build();
    joy_panel_build();
    cam_decorations_build();
    theme_menu_build();
    config_panel_build();   // last — layers above everything when open
}

void scout_ui_init(void)
{
    build_ui();
}

// Theme switch — recolour the shared styles, re-bake the crosshatch tile,
// then coordinate each module that owns themed widget colours.
void scout_ui_set_theme(uint8_t idx)
{
    idx %= THEME_COUNT;
    s_th = &s_themes[idx];

    styles_apply_theme();
    make_stripe_tile();
    lv_img_cache_invalidate_src(&s_stripe_tile);
    lv_obj_report_style_change(NULL);

    topbar_refresh_theme();
    botbar_set_theme_name(s_th->name);
    menu_set_active(idx);

    lv_obj_invalidate(lv_scr_act());
}
