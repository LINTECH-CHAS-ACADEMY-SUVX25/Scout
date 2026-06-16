#pragma once
#include "lvgl.h"
#include <stdint.h>

// Theme palette — one entry per selectable theme. The COL_* macros below read
// from s_th (the active pointer) so widget builders stay palette-agnostic.
typedef struct {
    const char *name;
    uint32_t bg, bar, panel, line, accent, accent_deep;
    uint32_t text_hi, text_mid, text_lo, good, bad;
    uint32_t badge_bg, badge_on;
} ui_theme_t;

extern const ui_theme_t  s_themes[];
extern const ui_theme_t *s_th;
#define THEME_COUNT 4

// Active-theme colour accessors
#define COL_BG          (s_th->bg)
#define COL_BAR         (s_th->bar)
#define COL_PANEL       (s_th->panel)
#define COL_LINE        (s_th->line)
#define COL_ACCENT      (s_th->accent)
#define COL_ACCENT_DEEP (s_th->accent_deep)
#define COL_TEXT_HI     (s_th->text_hi)
#define COL_TEXT_MID    (s_th->text_mid)
#define COL_TEXT_LO     (s_th->text_lo)
#define COL_GOOD        (s_th->good)
#define COL_BAD         (s_th->bad)
#define COL_BADGE_BG    (s_th->badge_bg)
#define COL_BADGE_ON    (s_th->badge_on)

// Fonts
LV_FONT_DECLARE(press_start_2p_8);
LV_FONT_DECLARE(press_start_2p_24);
LV_FONT_DECLARE(press_start_2p_96);
#define UI_FONT    (&press_start_2p_8)
#define SCENE_FONT (&press_start_2p_24)
#define LOGO_FONT  (&press_start_2p_96)

// Shared styles — the design vocabulary used by every widget builder.
// styles_init() sets geometry once; styles_apply_theme() reloads colours.
extern lv_style_t st_reset;
extern lv_style_t st_fill_bg;
extern lv_style_t st_fill_line;
extern lv_style_t st_fill_accent;
extern lv_style_t st_fill_textlo;
extern lv_style_t st_fill_texthi;
extern lv_style_t st_bar;
extern lv_style_t st_panel;
extern lv_style_t st_card;
extern lv_style_t st_chip;
extern lv_style_t st_rule;
extern lv_style_t st_btn;
extern lv_style_t st_menu_item;
extern lv_style_t st_pressed;
extern lv_style_t st_border_line;
extern lv_style_t st_border_accent;
extern lv_style_t st_border_texthi;
extern lv_style_t st_fg_hi;
extern lv_style_t st_fg_mid;
extern lv_style_t st_fg_lo;
extern lv_style_t st_fg_accent;
extern lv_style_t st_font_sm;
extern lv_style_t st_font_scene;
extern lv_style_t st_font_logo;
extern lv_style_t st_slider_main;
extern lv_style_t st_slider_ind;
extern lv_style_t st_slider_knob;
extern lv_style_t st_fill_bad;
extern lv_style_t st_fill_good;
extern lv_style_t st_chip_on;

// Crosshatch tile used as panel background texture.
extern lv_img_dsc_t s_stripe_tile;

void styles_init(void);
void styles_apply_theme(void);
void make_stripe_tile(void);
