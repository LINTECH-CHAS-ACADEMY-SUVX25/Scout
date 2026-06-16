#include "themes.h"

#define STRIPE_TILE 8
#define STRIPE_W    1
#define STRIPE_OPA  60

const ui_theme_t s_themes[THEME_COUNT] = {
    { .name = "SONAR",
      .bg = 0x0A0E14, .bar = 0x0D1117, .panel = 0x141A23, .line = 0x232B36,
      .accent = 0x22D3EE, .accent_deep = 0x0891B2,
      .text_hi = 0xE6EDF3, .text_mid = 0x9BA7B4, .text_lo = 0x5C6773,
      .good = 0x34D399, .bad = 0xF87171,
      .badge_bg = 0x1B2230, .badge_on = 0x10222A },
    { .name = "DESERT",
      .bg = 0x120C05, .bar = 0x170F07, .panel = 0x211609, .line = 0x3B2C16,
      .accent = 0xFFB454, .accent_deep = 0xB45309,
      .text_hi = 0xF3EAD9, .text_mid = 0xB7A88C, .text_lo = 0x77654A,
      .good = 0x34D399, .bad = 0xF87171,
      .badge_bg = 0x2B1F0E, .badge_on = 0x2A1C08 },
    { .name = "NIGHT OPS",
      .bg = 0x06100A, .bar = 0x09140D, .panel = 0x0E1F14, .line = 0x1F3A29,
      .accent = 0x4ADE80, .accent_deep = 0x15803D,
      .text_hi = 0xE3F2E8, .text_mid = 0x96B7A1, .text_lo = 0x567A64,
      .good = 0x34D399, .bad = 0xF87171,
      .badge_bg = 0x12291B, .badge_on = 0x0E2415 },
    { .name = "ARCTIC",
      .bg = 0xEFF4F8, .bar = 0xE4EBF3, .panel = 0xD8E2EE, .line = 0xA6BCCC,
      .accent = 0x0077B6, .accent_deep = 0x005F8F,
      .text_hi = 0x0A1628, .text_mid = 0x2A4260, .text_lo = 0x587A90,
      .good = 0x059669, .bad = 0xDC2626,
      .badge_bg = 0xC8D6E4, .badge_on = 0xB5C4D8 },
};

const ui_theme_t *s_th = &s_themes[0];

lv_style_t st_reset;
lv_style_t st_fill_bg;
lv_style_t st_fill_line;
lv_style_t st_fill_accent;
lv_style_t st_fill_textlo;
lv_style_t st_fill_texthi;
lv_style_t st_bar;
lv_style_t st_panel;
lv_style_t st_card;
lv_style_t st_chip;
lv_style_t st_rule;
lv_style_t st_btn;
lv_style_t st_menu_item;
lv_style_t st_pressed;
lv_style_t st_border_line;
lv_style_t st_border_accent;
lv_style_t st_border_texthi;
lv_style_t st_fg_hi;
lv_style_t st_fg_mid;
lv_style_t st_fg_lo;
lv_style_t st_fg_accent;
lv_style_t st_font_sm;
lv_style_t st_font_scene;
lv_style_t st_font_logo;
lv_style_t st_slider_main;
lv_style_t st_slider_ind;
lv_style_t st_slider_knob;
lv_style_t st_fill_bad;
lv_style_t st_fill_good;
lv_style_t st_chip_on;

static uint8_t      s_stripe_map[STRIPE_TILE * STRIPE_TILE * 3];
lv_img_dsc_t        s_stripe_tile;

void make_stripe_tile(void)
{
    lv_color_t line = lv_color_hex(COL_LINE);
    for(int y = 0; y < STRIPE_TILE; y++) {
        for(int x = 0; x < STRIPE_TILE; x++) {
            int i = (y * STRIPE_TILE + x) * 3;
            bool fwd  = (x + y) % STRIPE_TILE < STRIPE_W;
            bool back = (x - y + STRIPE_TILE) % STRIPE_TILE < STRIPE_W;
            s_stripe_map[i]     = line.full & 0xFF;
            s_stripe_map[i + 1] = line.full >> 8;
            s_stripe_map[i + 2] = (fwd || back) ? STRIPE_OPA : 0;
        }
    }
    s_stripe_tile.header.cf          = LV_IMG_CF_TRUE_COLOR_ALPHA;
    s_stripe_tile.header.always_zero = 0;
    s_stripe_tile.header.w           = STRIPE_TILE;
    s_stripe_tile.header.h           = STRIPE_TILE;
    s_stripe_tile.data_size          = sizeof(s_stripe_map);
    s_stripe_tile.data               = s_stripe_map;
}

void styles_init(void)
{
    static bool ready;
    if(ready) return;
    ready = true;

    lv_style_init(&st_reset);
    lv_style_set_radius(&st_reset, 0);
    lv_style_set_border_width(&st_reset, 0);
    lv_style_set_pad_all(&st_reset, 0);
    lv_style_set_bg_opa(&st_reset, LV_OPA_TRANSP);

    lv_style_init(&st_fill_bg);     lv_style_set_bg_opa(&st_fill_bg, LV_OPA_COVER);
    lv_style_init(&st_fill_line);   lv_style_set_bg_opa(&st_fill_line, LV_OPA_COVER);
    lv_style_init(&st_fill_accent); lv_style_set_bg_opa(&st_fill_accent, LV_OPA_COVER);
    lv_style_init(&st_fill_textlo); lv_style_set_bg_opa(&st_fill_textlo, LV_OPA_COVER);
    lv_style_init(&st_fill_texthi); lv_style_set_bg_opa(&st_fill_texthi, LV_OPA_COVER);

    lv_style_init(&st_bar);
    lv_style_set_radius(&st_bar, 8);
    lv_style_set_bg_opa(&st_bar, LV_OPA_COVER);
    lv_style_set_border_width(&st_bar, 1);

    lv_style_init(&st_panel);
    lv_style_set_radius(&st_panel, 8);
    lv_style_set_bg_opa(&st_panel, LV_OPA_COVER);
    lv_style_set_border_width(&st_panel, 1);
    lv_style_set_bg_img_src(&st_panel, &s_stripe_tile);
    lv_style_set_bg_img_tiled(&st_panel, true);

    lv_style_init(&st_card);
    lv_style_set_radius(&st_card, 8);
    lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
    lv_style_set_border_width(&st_card, 1);
    lv_style_set_pad_all(&st_card, 0);

    lv_style_init(&st_chip);
    lv_style_set_radius(&st_chip, 10);
    lv_style_set_bg_opa(&st_chip, LV_OPA_COVER);
    lv_style_set_border_width(&st_chip, 1);
    lv_style_set_pad_all(&st_chip, 0);

    lv_style_init(&st_rule);
    lv_style_set_bg_grad_dir(&st_rule, LV_GRAD_DIR_HOR);
    lv_style_set_bg_opa(&st_rule, LV_OPA_60);

    lv_style_init(&st_btn);
    lv_style_set_radius(&st_btn, 6);
    lv_style_set_bg_opa(&st_btn, LV_OPA_COVER);
    lv_style_set_border_width(&st_btn, 1);
    lv_style_set_pad_all(&st_btn, 0);

    lv_style_init(&st_menu_item);
    lv_style_set_radius(&st_menu_item, 6);
    lv_style_set_border_width(&st_menu_item, 0);
    lv_style_set_pad_all(&st_menu_item, 0);
    lv_style_set_bg_opa(&st_menu_item, LV_OPA_TRANSP);

    lv_style_init(&st_pressed);
    lv_style_set_bg_opa(&st_pressed, LV_OPA_COVER);

    lv_style_init(&st_border_line);   lv_style_set_border_opa(&st_border_line, LV_OPA_COVER);
    lv_style_init(&st_border_accent); lv_style_set_border_opa(&st_border_accent, LV_OPA_COVER);
    lv_style_init(&st_border_texthi); lv_style_set_border_opa(&st_border_texthi, LV_OPA_40);

    lv_style_init(&st_fg_hi);
    lv_style_init(&st_fg_mid);
    lv_style_init(&st_fg_lo);
    lv_style_init(&st_fg_accent);

    lv_style_init(&st_font_sm);    lv_style_set_text_font(&st_font_sm, UI_FONT);
    lv_style_init(&st_font_scene); lv_style_set_text_font(&st_font_scene, SCENE_FONT);
    lv_style_init(&st_font_logo);  lv_style_set_text_font(&st_font_logo, LOGO_FONT);

    lv_style_init(&st_slider_main);
    lv_style_set_radius(&st_slider_main, LV_RADIUS_CIRCLE);
    lv_style_set_bg_opa(&st_slider_main, LV_OPA_COVER);
    lv_style_set_border_width(&st_slider_main, 0);
    lv_style_set_pad_all(&st_slider_main, 0);

    lv_style_init(&st_slider_ind);
    lv_style_set_radius(&st_slider_ind, LV_RADIUS_CIRCLE);
    lv_style_set_bg_grad_dir(&st_slider_ind, LV_GRAD_DIR_HOR);
    lv_style_set_bg_opa(&st_slider_ind, LV_OPA_COVER);

    lv_style_init(&st_slider_knob);
    lv_style_set_radius(&st_slider_knob, LV_RADIUS_CIRCLE);
    lv_style_set_bg_opa(&st_slider_knob, LV_OPA_COVER);
    lv_style_set_pad_all(&st_slider_knob, 2);
    lv_style_set_border_width(&st_slider_knob, 1);

    lv_style_init(&st_fill_bad);  lv_style_set_bg_opa(&st_fill_bad,  LV_OPA_COVER);
    lv_style_init(&st_fill_good); lv_style_set_bg_opa(&st_fill_good, LV_OPA_COVER);

    lv_style_init(&st_chip_on);
    lv_style_set_bg_opa(&st_chip_on, LV_OPA_COVER);
    lv_style_set_border_width(&st_chip_on, 1);
    lv_style_set_border_opa(&st_chip_on, LV_OPA_COVER);
}

void styles_apply_theme(void)
{
    lv_style_set_bg_color(&st_fill_bg,     lv_color_hex(COL_BG));
    lv_style_set_bg_color(&st_fill_line,   lv_color_hex(COL_LINE));
    lv_style_set_bg_color(&st_fill_accent, lv_color_hex(COL_ACCENT));
    lv_style_set_bg_color(&st_fill_textlo, lv_color_hex(COL_TEXT_LO));
    lv_style_set_bg_color(&st_fill_texthi, lv_color_hex(COL_TEXT_HI));

    lv_style_set_bg_color(&st_bar,     lv_color_hex(COL_BAR));
    lv_style_set_border_color(&st_bar, lv_color_hex(COL_LINE));

    lv_style_set_bg_color(&st_panel,     lv_color_hex(COL_PANEL));
    lv_style_set_border_color(&st_panel, lv_color_hex(COL_LINE));

    lv_style_set_bg_color(&st_card,     lv_color_hex(COL_BAR));
    lv_style_set_border_color(&st_card, lv_color_hex(COL_LINE));

    lv_style_set_bg_color(&st_chip,     lv_color_hex(COL_BADGE_BG));
    lv_style_set_border_color(&st_chip, lv_color_hex(COL_LINE));

    lv_style_set_bg_color(&st_rule,      lv_color_hex(COL_ACCENT));
    lv_style_set_bg_grad_color(&st_rule, lv_color_hex(COL_PANEL));

    lv_style_set_bg_color(&st_btn,     lv_color_hex(COL_BADGE_BG));
    lv_style_set_border_color(&st_btn, lv_color_hex(COL_ACCENT));

    lv_style_set_bg_color(&st_pressed, lv_color_hex(COL_BADGE_ON));

    lv_style_set_border_color(&st_border_line,   lv_color_hex(COL_LINE));
    lv_style_set_border_color(&st_border_accent, lv_color_hex(COL_ACCENT));
    lv_style_set_border_color(&st_border_texthi, lv_color_hex(COL_TEXT_HI));

    lv_style_set_text_color(&st_fg_hi,     lv_color_hex(COL_TEXT_HI));
    lv_style_set_text_color(&st_fg_mid,    lv_color_hex(COL_TEXT_MID));
    lv_style_set_text_color(&st_fg_lo,     lv_color_hex(COL_TEXT_LO));
    lv_style_set_text_color(&st_fg_accent, lv_color_hex(COL_ACCENT));

    lv_style_set_bg_color(&st_slider_main,      lv_color_hex(COL_LINE));
    lv_style_set_bg_color(&st_slider_ind,       lv_color_hex(COL_ACCENT_DEEP));
    lv_style_set_bg_grad_color(&st_slider_ind,  lv_color_hex(COL_ACCENT));
    lv_style_set_bg_color(&st_slider_knob,      lv_color_hex(COL_ACCENT));
    lv_style_set_border_color(&st_slider_knob,  lv_color_hex(COL_PANEL));

    lv_style_set_bg_color(&st_fill_bad,  lv_color_hex(COL_BAD));
    lv_style_set_bg_color(&st_fill_good, lv_color_hex(COL_GOOD));

    lv_style_set_bg_color(&st_chip_on,     lv_color_hex(COL_BADGE_ON));
    lv_style_set_border_color(&st_chip_on, lv_color_hex(COL_ACCENT));
}
