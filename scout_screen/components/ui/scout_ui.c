#include "scout_ui.h"
#include "display.h"
#include "rc_protocol.h"
#include "lvgl.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Owns the full UI layout — widget creation, event callbacks, and the intro
// overlay. Compiled by both the scout_screen firmware and the PC simulator
// (sim/), so it must stay free of ESP-IDF and FreeRTOS headers. The simulator
// provides its own display.h with SCREEN_W/SCREEN_H.
//
// Appearance is driven by a small design system of shared lv_style_t objects
// (see "Shared styles" below) rather than per-widget style calls. Switching
// theme reloads the colours into those styles and asks LVGL to refresh — no
// widget is destroyed or rebuilt.

// Max knob offset from centre — base half (65) minus halo half (31), so
// neither the knob nor its halo ever leaves the 130px joystick frame.
#define JOY_RADIUS 34
#define JOY_DEADZONE 15   // px from centre before a direction counts as pressed

// Intro overlay loading bar — slim rounded track with a gradient fill.
// The fill sits inside the track's 1px border + 2px padding (3px inset/side).
#define INTRO_BAR_W   440
#define INTRO_BAR_H   10
#define INTRO_FILL_W  (INTRO_BAR_W - 6)
#define INTRO_FILL_H  (INTRO_BAR_H - 6)
#define INTRO_HOLD_MS 1200   // how long the finished bar stays before the overlay closes

// Intro layout, all measured from the screen centre. The hero (wordmark +
// divider + subtitle) sits above a status line and the loading bar; corner
// brackets and header/footer hairlines frame it like the rest of the UI.
#define INTRO_LOGO_Y    (-96)   // wordmark baseline offset
#define INTRO_RULE_Y    (-26)   // accent divider under the wordmark
#define INTRO_SUB_Y     (-4)    // subtitle under the divider
#define INTRO_DOTS_Y    64      // boot-step dots
#define INTRO_BAR_Y     96      // loading bar
#define INTRO_TEXT_Y    122     // status + percentage row
#define INTRO_RULE_W    300     // accent divider width
#define INTRO_FRAME     28      // inset of the corner brackets from the screen edge
#define INTRO_CORNER    34      // corner bracket arm length
#define INTRO_TAG_DY    (INTRO_FRAME + INTRO_CORNER / 2 - 4) // header/footer text, vertically centred on the bracket
#define INTRO_DOT_GAP   22      // spacing between boot-step dots
#define INTRO_MAX_STEPS 12      // cap on dots built in scout_ui_intro_screen

// Layout — a left sidebar flanks the centred camera area. BAR_H is the same
// for top and bottom so the camera sits vertically symmetric.
#define BAR_H       36
#define PANEL_GAP   8                   // air between the cards/bars and the screen edges
#define CONTENT_Y   (BAR_H + PANEL_GAP) // content starts below the floating topbar
#define CONTENT_H   (SCREEN_H - 2 * CONTENT_Y)
#define SIDE_W      240
#define PAD         14
#define ROW_W       (SIDE_W - 2 * PAD)
#define HDR_Y       12                  // section header at the top of each panel
#define TELE_CARD_Y 78                  // centres the card in the space below the header
#define TELE_PITCH  30                  // row spacing in the telemetry

// Command badges (FWD/BWD/STP/LFT/RGT)
#define BADGE_W     38
#define BADGE_H     20
#define BADGE_GAP   7
#define BADGE_STEP  (BADGE_W + BADGE_GAP)

// Themes dropdown — card under the topbar's right side
#define MENU_W      150
#define MENU_ITEM_H 34
#define MENU_PAD    6

// Telemetry card — rows sit inside a raised card with hairline separators
#define CARD_PAD    12
#define TELE_ROW_W  (ROW_W - 2 * CARD_PAD - 2)
#define TELE_CARD_H (2 * TELE_PITCH + 22 + 2 * (CARD_PAD + 1))

// Floating corner panels — telemetry top right, joystick bottom left,
// both the same size for visual symmetry
#define PANEL_H 236

// Crosshatch texture tiled across the card backgrounds — diagonals in both
// directions form a diamond pattern. The lines are COL_LINE at low opacity
// so the same tile works on any card colour.
#define STRIPE_TILE 8                   // tile side and line period; wraps seamlessly
#define STRIPE_W    1                   // line thickness along x
#define STRIPE_OPA  60                  // line alpha (0-255)

// Camera viewfinder — accent corner brackets just outside the video area
#define CAM_X       ((SCREEN_W - CAM_W) / 2)
#define CAM_Y       ((SCREEN_H - CAM_H) / 2)
#define CAM_GAP     8
#define CAM_CORNER  22

// Idle joystick knob — a fixed slate grey, identical across all themes.
#define KNOB_IDLE   0x3A434F

// Palette — dark tech / premium. One entry per selectable theme in the
// THEMES dropdown; the COL_* macros below read from the active theme. The
// shared styles are (re)loaded from these colours, so the widget builders
// stay palette-agnostic and a theme switch never rebuilds the UI.
typedef struct {
    const char *name;
    uint32_t bg, bar, panel, line, accent, accent_deep;
    uint32_t text_hi, text_mid, text_lo, good, bad;
    uint32_t badge_bg, badge_on;
} ui_theme_t;

static const ui_theme_t s_themes[] = {
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
};
#define THEME_COUNT (sizeof(s_themes) / sizeof(s_themes[0]))

static const ui_theme_t *s_th = &s_themes[0];

#define COL_BG          (s_th->bg)
#define COL_BAR         (s_th->bar)
#define COL_PANEL       (s_th->panel)
#define COL_LINE        (s_th->line)
#define COL_ACCENT      (s_th->accent)
#define COL_ACCENT_DEEP (s_th->accent_deep)   // gradient start for the loading fill
#define COL_TEXT_HI     (s_th->text_hi)
#define COL_TEXT_MID    (s_th->text_mid)
#define COL_TEXT_LO     (s_th->text_lo)
#define COL_GOOD        (s_th->good)
#define COL_BAD         (s_th->bad)
#define COL_BADGE_BG    (s_th->badge_bg)
#define COL_BADGE_ON    (s_th->badge_on)

#define XSTR(x) STR(x)
#define STR(x)  #x

// UI font — Press Start 2P (generated C fonts, see the respective .c files).
// The 96px logo font only contains space + A-Z to keep its size down.
LV_FONT_DECLARE(press_start_2p_8);
LV_FONT_DECLARE(press_start_2p_24);
LV_FONT_DECLARE(press_start_2p_96);
#define UI_FONT    (&press_start_2p_8)
#define SCENE_FONT (&press_start_2p_24)  // scene overlay text over the camera region
#define LOGO_FONT  (&press_start_2p_96)  // intro logo

static volatile int16_t s_joy_x;
static volatile int16_t s_joy_y;
static uint8_t          s_last_cmd = 0xFF;

// Widget handles

static lv_obj_t *s_root;          // holds the whole UI; persists across theme switches
static lv_obj_t *s_theme_menu;
static uint8_t   s_pending_theme;
static uint8_t   s_wifi_level;
static uint8_t   s_cmd = CMD_STOP;     // last command, re-applied after a theme switch
static lv_obj_t *s_intro_overlay;
static lv_obj_t *s_intro_bar_fill;
static lv_obj_t *s_intro_status;
static lv_obj_t *s_intro_pct;
static lv_obj_t *s_intro_dots[INTRO_MAX_STEPS]; // one per boot step, lit as it completes
static uint8_t   s_intro_total;
static uint8_t   s_intro_step;
static lv_obj_t *s_knob;
static lv_obj_t *s_halo;
static lv_obj_t *s_wifi_dot;
static lv_obj_t *s_wifi_arcs[3];
static lv_obj_t *s_wifi_slash;
static lv_obj_t *s_link_dot;
static lv_obj_t *s_link_lbl;
static lv_obj_t *s_cmd_badges[5];
static lv_obj_t *s_cmd_badge_lbls[5];   // labels recolour with badge state; kept to avoid child lookups
static lv_obj_t *s_val_temp;
static lv_obj_t *s_val_humi;
static lv_obj_t *s_val_pres;
static lv_obj_t *s_scene_overlay;       // covers the camera region; driven by scout_ui_overlay()
static lv_obj_t *s_scene_label;
static const char *s_overlay_text;
static lv_obj_t *s_theme_name_lbl;          // bottom bar active-theme name
static lv_obj_t *s_theme_dots[THEME_COUNT]; // dropdown active markers, one per item

static lv_obj_t *s_cfg_panel;     // CONFIG — full-height camera-control panel on the right
static bool      s_config_open;

static void fmt_temp(char *out, size_t n, const cam_diag_pkt_t *d)
{
    int t     = d->temp_cdeg;
    int whole = t / 100;
    int frac  = (t < 0 ? -t : t) % 100 / 10;
    const char *sign = (t < 0 && whole == 0) ? "-" : "";
    snprintf(out, n, "%s%d.%d C", sign, whole, frac);
}

static void fmt_humi(char *out, size_t n, const cam_diag_pkt_t *d)
{
    snprintf(out, n, "%u %%", (unsigned)d->humidity_pct);
}

static void fmt_pres(char *out, size_t n, const cam_diag_pkt_t *d)
{
    snprintf(out, n, "%lu hPa", (unsigned long)(d->pressure_pa / 100));
}

// Telemetry readouts: each row pairs a value label with the formatter for its field.
// last holds the text currently shown, so unchanged fields skip the LVGL repaint.
typedef struct {
    lv_obj_t **widget;
    void      (*fmt)(char *out, size_t n, const cam_diag_pkt_t *d);
    char       last[16];
} tele_field_t;

static tele_field_t s_tele[] = {
    { &s_val_temp, fmt_temp, "" },
    { &s_val_humi, fmt_humi, "" },
    { &s_val_pres, fmt_pres, "" },
};

static cam_diag_pkt_t s_cam_diag;       // last packet, re-applied after a theme rebuild

// ---------------------------------------------------------------------------
// Shared styles — the design system
//
// Each role the UI repeats (a floating bar, a panel, a chip, a hairline, a
// text colour, a slider part …) is a single lv_style_t shared by every widget
// playing that role. styles_init() runs once and fixes the geometry that never
// changes (radius, border widths, fonts, the crosshatch image); it is the
// design vocabulary. styles_apply_theme() (re)loads every colour from the
// active theme; it is the only thing a theme switch has to do to the styles.
// ---------------------------------------------------------------------------

static lv_style_t st_reset;          // neutralises the default theme on plain containers
static lv_style_t st_fill_bg;        // solid background-coloured fill
static lv_style_t st_fill_line;      // hairline / separator fill
static lv_style_t st_fill_accent;    // accent-coloured fill (marks, dots, swatch)
static lv_style_t st_fill_textlo;    // text-low coloured fill (joystick ticks)
static lv_style_t st_fill_texthi;    // text-high coloured fill (crosshair dot)
static lv_style_t st_bar;            // floating top/bottom bar
static lv_style_t st_panel;          // floating panel with crosshatch texture
static lv_style_t st_card;           // raised inner card / dropdown body
static lv_style_t st_chip;           // pill chip + command badge body
static lv_style_t st_rule;           // section-header gradient rule
static lv_style_t st_btn;            // APPLY button body
static lv_style_t st_menu_item;      // theme dropdown row body
static lv_style_t st_pressed;        // pressed-state fill for buttons / menu items
static lv_style_t st_border_line;    // themed line-coloured border (width set locally)
static lv_style_t st_border_accent;  // themed accent-coloured border (width set locally)
static lv_style_t st_border_texthi;  // themed text-high border (crosshair ring)
static lv_style_t st_fg_hi;          // text colour: high-emphasis
static lv_style_t st_fg_mid;         // text colour: mid
static lv_style_t st_fg_lo;          // text colour: low
static lv_style_t st_fg_accent;      // text colour: accent
static lv_style_t st_font_sm;        // small UI font
static lv_style_t st_font_scene;     // scene-overlay font
static lv_style_t st_font_logo;      // intro wordmark font
static lv_style_t st_slider_main;    // slider track
static lv_style_t st_slider_ind;     // slider indicator (accent gradient)
static lv_style_t st_slider_knob;    // slider knob
static bool       s_styles_ready;

// Stripe tile pixel data — RGB565 + alpha, filled in by make_stripe_tile().
static uint8_t      s_stripe_map[STRIPE_TILE * STRIPE_TILE * 3];
static lv_img_dsc_t s_stripe_tile;

// Builds the crosshatch tile: (x + y) wrapping inside STRIPE_W gives the "/"
// diagonals, (x - y) the "\" ones; everything else stays transparent. The line
// colour is baked into the pixels, so a theme switch re-bakes the tile.
static void make_stripe_tile(void)
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

// One-time geometry. Runs before any widget is built; never again.
static void styles_init(void)
{
    if(s_styles_ready) return;
    s_styles_ready = true;

    // Plain container: strip the default theme's frame so the object is an
    // invisible box until something layers a fill/border style on top.
    lv_style_init(&st_reset);
    lv_style_set_radius(&st_reset, 0);
    lv_style_set_border_width(&st_reset, 0);
    lv_style_set_pad_all(&st_reset, 0);
    lv_style_set_bg_opa(&st_reset, LV_OPA_TRANSP);

    // Solid fills — colour comes from the theme, opacity is fixed here.
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
}

// Loads every theme-dependent colour into the shared styles. Called at init and
// on each theme switch; it never touches geometry.
static void styles_apply_theme(void)
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
}

// Widget helpers

// Plain container: default theme stripped, not scrollable or clickable.
static lv_obj_t *make_obj(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_add_style(o, &st_reset, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

// Label with a foreground-colour style (NULL for runtime-coloured labels) and
// a font style (NULL defaults to the small UI font).
static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                            lv_style_t *fg, lv_style_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    if(fg) lv_obj_add_style(l, fg, 0);
    lv_obj_add_style(l, font ? font : &st_font_sm, 0);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
    return l;
}

// Thin vertical hairline used to separate groups in the top and bottom bars.
static void make_vsep(lv_obj_t *parent, lv_align_t align, int32_t x)
{
    lv_obj_t *s = make_obj(parent);
    lv_obj_add_style(s, &st_fill_line, 0);
    lv_obj_set_size(s, 1, 12);
    lv_obj_align(s, align, x, 0);
}

// One arc of the WiFi signal symbol — a 90° fan segment above the dot,
// all arcs sharing the same centre point (cx, cy).
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

// Accent corner bracket: an L of two borders. Used by both the camera
// viewfinder (parent = s_root) and the intro overlay.
static lv_obj_t *make_corner(lv_obj_t *parent, int32_t x, int32_t y,
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

// Floating rounded panel — the shared card style for the corner widgets.
static lv_obj_t *make_panel(int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *p = make_obj(s_root);
    lv_obj_add_style(p, &st_panel, 0);
    lv_obj_set_size(p, w, h);
    lv_obj_set_pos(p, x, y);
    return p;
}

// Section header with an accent mark on the left and a gradient rule that
// fades out across the panel below the text.
static void make_section_hdr(lv_obj_t *parent, const char *text, int32_t y)
{
    // y-3 centres the 14px mark on the 8px text's midline (y+4)
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

static lv_obj_t *create_joy_tick(lv_obj_t *parent, lv_align_t align,
                                   int32_t xo, int32_t yo, bool horiz)
{
    lv_obj_t *t = make_obj(parent);
    lv_obj_add_style(t, &st_fill_textlo, 0);
    lv_obj_set_size(t, horiz ? 7 : 1, horiz ? 1 : 7);
    lv_obj_align(t, align, xo, yo);
    lv_obj_set_style_radius(t, 1, 0);
    return t;
}

static lv_obj_t *make_tele_row(lv_obj_t *parent, const char *key,
                                 const char *init_val, int32_t y)
{
    lv_obj_t *row = make_obj(parent);
    lv_obj_set_size(row, TELE_ROW_W, 22);
    lv_obj_set_pos(row, 0, y);

    lv_obj_t *k = make_label(row, key, &st_fg_mid, NULL);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 0, 0);

    // Value sits in a raised chip so the readouts read as instruments
    lv_obj_t *chip = make_obj(row);
    lv_obj_add_style(chip, &st_chip, 0);
    lv_obj_set_size(chip, 84, 20);
    lv_obj_align(chip, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *v = make_label(chip, init_val, &st_fg_accent, NULL);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, -8, 0);
    return v;
}

// Command badges

static void update_cmd_badges(uint8_t cmd)
{
    if(cmd == s_last_cmd) return;
    s_last_cmd = cmd;

    static const uint8_t masks[5] = {
        CMD_FORWARD, CMD_BACKWARD, 0xFF, CMD_LEFT, CMD_RIGHT
    };
    s_cmd = cmd;
    for(int i = 0; i < 5; i++) {
        bool active = (i == 2) ? (cmd == CMD_STOP) : (cmd & masks[i]);
        lv_obj_set_style_bg_color(s_cmd_badges[i],
            lv_color_hex(active ? COL_BADGE_ON : COL_BADGE_BG), 0);
        lv_obj_set_style_border_color(s_cmd_badges[i],
            lv_color_hex(active ? COL_ACCENT : COL_LINE), 0);
        lv_obj_set_style_text_color(s_cmd_badge_lbls[i],
            lv_color_hex(active ? COL_ACCENT : COL_TEXT_LO), 0);
    }
}

// Joystick event

static void joy_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t       *base = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t  pt;
        lv_indev_get_point(indev, &pt);

        lv_area_t coords;
        lv_obj_get_coords(base, &coords);
        int dx = pt.x - (coords.x1 + coords.x2) / 2;
        int dy = pt.y - (coords.y1 + coords.y2) / 2;

        float mag = sqrtf((float)(dx * dx + dy * dy));
        if(mag > JOY_RADIUS) {
            dx = (int)(dx * JOY_RADIUS / mag);
            dy = (int)(dy * JOY_RADIUS / mag);
        }

        lv_obj_align(s_knob, LV_ALIGN_CENTER, dx, dy);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, dx, dy);
        lv_obj_set_style_transform_zoom(s_knob, 210, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(COL_ACCENT), 0);

        s_joy_x =  (int16_t)((dx * 255) / JOY_RADIUS);
        s_joy_y = -(int16_t)((dy * 255) / JOY_RADIUS);

        uint8_t cmd = CMD_STOP;
        if(dy < -JOY_DEADZONE) cmd |= CMD_FORWARD;
        if(dy >  JOY_DEADZONE) cmd |= CMD_BACKWARD;
        if(dx < -JOY_DEADZONE) cmd |= CMD_LEFT;
        if(dx >  JOY_DEADZONE) cmd |= CMD_RIGHT;
        update_cmd_badges(cmd);
    } else {
        lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_transform_zoom(s_knob, 256, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(KNOB_IDLE), 0);
        s_joy_x = 0;
        s_joy_y = 0;
        update_cmd_badges(CMD_STOP);
    }
}

// Intro close timer — one-shot, fires after the last init step so the full
// bar stays visible a moment. Runs inside the render loop's lv_timer_handler.

static void intro_close_cb(lv_timer_t *t)
{
    (void)t;
    lv_obj_del(s_intro_overlay);
    s_intro_overlay = NULL;
}

// Themes dropdown events — selecting a theme recolours the shared styles. The
// recolour runs from a one-shot timer, never from the item's own callback, to
// keep it off the event path even though nothing is destroyed any more.

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

static void themes_event(lv_event_t *e)
{
    (void)e;
    if(lv_obj_has_flag(s_theme_menu, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(s_theme_menu, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_theme_menu);
    } else {
        lv_obj_add_flag(s_theme_menu, LV_OBJ_FLAG_HIDDEN);
    }
}

// Config panel — full camera height, button pinned near the bottom.
#define CFG_PANEL_H CAM_H
#define CFG_BTN_H   24
#define CFG_BTN_Y   (CFG_PANEL_H - CFG_BTN_H - 16)

// APPLY closes the CONFIG panel.
static void panel_close_event(lv_event_t *e)
{
    (void)e;
    lv_obj_add_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);
    s_config_open = false;
}

// Toggles the CONFIG panel; registered to the CONFIG topbar label.
static void config_event(lv_event_t *e)
{
    (void)e;
    s_config_open = !s_config_open;
    if(s_config_open) {
        lv_obj_clear_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_cfg_panel);
    } else {
        lv_obj_add_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

// Slider value callback — receives the paired value label as user_data and
// reprints it as a plain signed integer when the slider moves.
static void slider_num_event(lv_event_t *e)
{
    lv_obj_t *lbl = lv_event_get_user_data(e);
    lv_label_set_text_fmt(lbl, "%d", (int)lv_slider_get_value(lv_event_get_target(e)));
}

// Accent-bordered APPLY button pinned to the bottom of a config panel.
static void make_apply_btn(lv_obj_t *panel)
{
    lv_obj_t *btn = lv_obj_create(panel);
    lv_obj_add_style(btn, &st_btn, 0);
    lv_obj_add_style(btn, &st_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(btn, ROW_W, CFG_BTN_H);
    lv_obj_set_pos(btn, PAD, CFG_BTN_Y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn, panel_close_event, LV_EVENT_CLICKED, panel);

    lv_obj_t *lbl = make_label(btn, "APPLY", &st_fg_accent, NULL);
    lv_obj_set_style_text_letter_space(lbl, 4, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
}

// Config slider row: key label left, live value label right, themed slider below.
// val_cb fires on LV_EVENT_VALUE_CHANGED and receives the value label as user_data.
static void make_slider_row(lv_obj_t *panel, const char *key, const char *init_val,
                             int32_t min, int32_t max, int32_t def,
                             int32_t y, lv_event_cb_t val_cb)
{
    lv_obj_t *k = make_label(panel, key, &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(k, 2, 0);
    lv_obj_set_pos(k, PAD, y);

    lv_obj_t *v = make_label(panel, init_val, &st_fg_accent, NULL);
    lv_obj_set_style_text_letter_space(v, 2, 0);
    lv_obj_set_width(v, 48);
    lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(v, PAD + ROW_W - 48, y);

    lv_obj_t *sl = lv_slider_create(panel);
    lv_obj_set_size(sl, ROW_W, 4);
    lv_obj_set_pos(sl, PAD, y + 15);
    lv_slider_set_range(sl, min, max);
    lv_slider_set_value(sl, def, LV_ANIM_OFF);

    // Track / indicator / knob each get their shared part style. The knob's
    // panel-coloured ring gives the "floating dot" look without bulk.
    lv_obj_add_style(sl, &st_slider_main, LV_PART_MAIN);
    lv_obj_add_style(sl, &st_slider_ind,  LV_PART_INDICATOR);
    lv_obj_add_style(sl, &st_slider_knob, LV_PART_KNOB);

    lv_obj_add_event_cb(sl, val_cb, LV_EVENT_VALUE_CHANGED, v);
}

// Config switch row: key label left, themed on/off switch right. The switch
// reuses the slider part styles so it reads as the same instrument family.
static lv_obj_t *make_switch_row(lv_obj_t *panel, const char *key, int32_t y, bool on)
{
    lv_obj_t *k = make_label(panel, key, &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(k, 2, 0);
    lv_obj_set_pos(k, PAD, y);

    lv_obj_t *sw = lv_switch_create(panel);
    lv_obj_set_size(sw, 40, 20);
    lv_obj_set_pos(sw, PAD + ROW_W - 40, y - 6);   // centre the switch on the label row
    lv_obj_add_style(sw, &st_slider_main, LV_PART_MAIN);
    lv_obj_add_style(sw, &st_slider_ind,  LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_style(sw, &st_slider_knob, LV_PART_KNOB);
    if(on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    return sw;
}

// Hairline separator between rows inside a config panel.
static void make_cfg_sep(lv_obj_t *panel, int32_t y)
{
    lv_obj_t *s = make_obj(panel);
    lv_obj_add_style(s, &st_fill_line, 0);
    lv_obj_set_size(s, ROW_W, 1);
    lv_obj_set_pos(s, PAD, y);
}

// UI assembly — one builder per visible module, called from scout_ui_init

// Topbar — floating card with the brand and the WiFi signal symbol.
static void make_topbar(void)
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

    // Link status in the centre — dot + label driven by scout_ui_update().
    // Both are coloured at runtime, so they carry no foreground style.
    s_link_dot = make_obj(topbar);
    lv_obj_set_size(s_link_dot, 6, 6);
    lv_obj_align(s_link_dot, LV_ALIGN_CENTER, -36, 0);
    lv_obj_set_style_radius(s_link_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_link_dot, lv_color_hex(COL_BAD), 0);
    lv_obj_set_style_bg_opa(s_link_dot, LV_OPA_COVER, 0);

    s_link_lbl = make_label(topbar, "NO LINK", NULL, NULL);
    lv_obj_set_style_text_color(s_link_lbl, lv_color_hex(COL_TEXT_LO), 0);
    lv_obj_set_style_text_letter_space(s_link_lbl, 2, 0);
    lv_obj_set_width(s_link_lbl, 80);
    lv_obj_set_style_text_align(s_link_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(s_link_lbl, LV_ALIGN_CENTER, 18, 0);

    // CONFIG opens the camera-control panel; sits left of THEMES
    lv_obj_t *cfg_lbl = make_label(topbar, "CONFIG", &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(cfg_lbl, 2, 0);
    lv_obj_align(cfg_lbl, LV_ALIGN_RIGHT_MID, -144, 0);
    lv_obj_add_flag(cfg_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(cfg_lbl, 10);
    lv_obj_add_event_cb(cfg_lbl, config_event, LV_EVENT_CLICKED, NULL);

    make_vsep(topbar, LV_ALIGN_RIGHT_MID, -130);

    // THEMES opens the theme dropdown — sits left of the separator
    lv_obj_t *themes = make_label(topbar, "THEMES", &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(themes, 2, 0);
    lv_obj_align(themes, LV_ALIGN_RIGHT_MID, -66, 0);
    lv_obj_add_flag(themes, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(themes, 10);   // the 8px label alone is a small tap target
    lv_obj_add_event_cb(themes, themes_event, LV_EVENT_CLICKED, NULL);

    make_vsep(topbar, LV_ALIGN_RIGHT_MID, -52);

    // WiFi signal symbol — dot + three arcs, lit according to the level
    // Container height matches the visible glyph (arc tops to dot bottom).
    // The -2 offset centres the symbol optically against the separator.
    lv_obj_t *wifi_icon = make_obj(topbar);
    lv_obj_set_size(wifi_icon, 28, 15);
    lv_obj_align(wifi_icon, LV_ALIGN_RIGHT_MID, -14, -2);

    s_wifi_dot = make_obj(wifi_icon);
    lv_obj_set_size(s_wifi_dot, 4, 4);
    lv_obj_set_pos(s_wifi_dot, 12, 11);
    lv_obj_set_style_radius(s_wifi_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_wifi_dot, lv_color_hex(COL_BAD), 0);
    lv_obj_set_style_bg_opa(s_wifi_dot, LV_OPA_COVER, 0);

    for(int i = 0; i < 3; i++) {
        s_wifi_arcs[i] = make_wifi_arc(wifi_icon, 10 + 8 * i, 14, 13);
    }

    static const lv_point_t slash_pts[2] = {{2, 14}, {26, 1}};
    s_wifi_slash = lv_line_create(wifi_icon);
    lv_line_set_points(s_wifi_slash, slash_pts, 2);
    lv_obj_set_style_line_color(s_wifi_slash, lv_color_hex(COL_BAD), 0);
    lv_obj_set_style_line_width(s_wifi_slash, 2, 0);
    lv_obj_add_flag(s_wifi_slash, LV_OBJ_FLAG_HIDDEN);
}

// Bottombar — floating card with the network facts and the RTOS tag.
static void make_botbar(void)
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

    // Active theme — accent swatch + name. The swatch tracks the accent via
    // st_fill_accent; the name text is updated by scout_ui_set_theme().
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

// Telemetry panel — floating card in the top left corner, aligned with the
// camera top edge. Holds the value rows in a raised inner card.
static void make_tele_panel(void)
{
    lv_obj_t *tele_panel = make_panel(PANEL_GAP,
                                      CAM_Y,
                                      SIDE_W, PANEL_H);

    make_section_hdr(tele_panel, "TELEMETRI", HDR_Y);

    // Raised card around the telemetry rows
    lv_obj_t *tele_card = make_obj(tele_panel);
    lv_obj_add_style(tele_card, &st_card, 0);
    lv_obj_set_size(tele_card, ROW_W, TELE_CARD_H);
    lv_obj_set_pos(tele_card, PAD, TELE_CARD_Y);

    lv_obj_t *tele_cont = make_obj(tele_card);
    lv_obj_set_size(tele_cont, TELE_ROW_W, 2 * TELE_PITCH + 22);
    lv_obj_set_pos(tele_cont, CARD_PAD, CARD_PAD);

    s_val_temp = make_tele_row(tele_cont, "temperature", "--.- C",   0 * TELE_PITCH);
    s_val_humi = make_tele_row(tele_cont, "humidity",    "-- %",     1 * TELE_PITCH);
    s_val_pres = make_tele_row(tele_cont, "pressure",    "---- hPa", 2 * TELE_PITCH);

    for(int i = 1; i < 3; i++) {
        lv_obj_t *sep = make_obj(tele_cont);
        lv_obj_add_style(sep, &st_fill_line, 0);
        lv_obj_set_size(sep, TELE_ROW_W, 1);
        lv_obj_set_pos(sep, 0, i * TELE_PITCH - 4);
    }
}

// Command badge row (FWD/BWD/STP/LFT/RGT) at the top of the joystick panel.
static void make_cmd_badges(lv_obj_t *joy_panel)
{
    const char *badge_labels[5] = { "FWD", "BWD", "STP", "LFT", "RGT" };
    int32_t badge_x = (SIDE_W - (5 * BADGE_W + 4 * BADGE_GAP)) / 2;   // centre the row
    for(int i = 0; i < 5; i++) {
        lv_obj_t *badge = make_obj(joy_panel);
        lv_obj_add_style(badge, &st_chip, 0);
        lv_obj_set_size(badge, BADGE_W, BADGE_H);
        lv_obj_set_pos(badge, badge_x, 44);
        lv_obj_t *bl = make_label(badge, badge_labels[i], NULL, NULL);
        lv_obj_align(bl, LV_ALIGN_CENTER, 0, 0);
        s_cmd_badges[i]     = badge;
        s_cmd_badge_lbls[i] = bl;
        badge_x += BADGE_STEP;
    }
    s_last_cmd = 0xFF;  // fresh widgets need a full paint regardless of prior state
    update_cmd_badges(CMD_STOP);
}

// The joystick itself — concentric rings, touch base, halo and knob.
static void make_joystick(lv_obj_t *joy_panel)
{
    // Outer concentric ring gives the joystick an instrument-dial look
    lv_obj_t *joy_ring = make_obj(joy_panel);
    lv_obj_add_style(joy_ring, &st_border_line, 0);
    lv_obj_set_size(joy_ring, 146, 146);
    lv_obj_set_pos(joy_ring, (SIDE_W - 146) / 2, 78);
    lv_obj_set_style_radius(joy_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(joy_ring, 1, 0);

    // Accent dots at the ring's diagonals complete the dial. 51 ≈ 72/√2,
    // the diagonal offset from the ring centre (73, 73).
    for(int dy = -1; dy <= 1; dy += 2) {
        for(int dx = -1; dx <= 1; dx += 2) {
            lv_obj_t *d = make_obj(joy_ring);
            lv_obj_add_style(d, &st_fill_accent, 0);
            lv_obj_set_size(d, 3, 3);
            lv_obj_set_pos(d, 73 + dx * 51 - 1, 73 + dy * 51 - 1);
            lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(d, LV_OPA_50, 0);
        }
    }

    // base keeps the default container padding on purpose: the edge ticks
    // below align to its content area, so the padding is their inset.
    lv_obj_t *base = lv_obj_create(joy_panel);
    lv_obj_add_style(base, &st_fill_bg, 0);
    lv_obj_add_style(base, &st_border_line, 0);
    lv_obj_set_size(base, 130, 130);
    lv_obj_set_pos(base, (SIDE_W - 130) / 2, 86);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(base, 2, 0);
    // Shadow removed — blurred shadow rendering into PSRAM framebuffers
    // stalled the LVGL render pass and tripped the task watchdog.
    lv_obj_clear_flag(base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESSING,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_RELEASED,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *guide = make_obj(base);
    lv_obj_add_style(guide, &st_border_line, 0);
    lv_obj_set_size(guide, 82, 82);
    lv_obj_align(guide, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(guide, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(guide, 1, 0);

    create_joy_tick(base, LV_ALIGN_TOP_MID,    0,  7, false);
    create_joy_tick(base, LV_ALIGN_BOTTOM_MID, 0, -7, false);
    create_joy_tick(base, LV_ALIGN_LEFT_MID,   7,  0, true);
    create_joy_tick(base, LV_ALIGN_RIGHT_MID, -7,  0, true);

    s_halo = make_obj(base);
    lv_obj_add_style(s_halo, &st_fill_accent, 0);
    lv_obj_set_size(s_halo, 62, 62);
    lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_halo, LV_OPA_20, 0);

    s_knob = make_obj(base);
    lv_obj_set_size(s_knob, 46, 46);
    lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_knob, lv_color_hex(KNOB_IDLE), 0);
    lv_obj_set_style_bg_opa(s_knob, LV_OPA_COVER, 0);
    // Shadow removed — same watchdog/stall reason as the base above.
    lv_obj_set_style_transform_pivot_x(s_knob, 23, 0);
    lv_obj_set_style_transform_pivot_y(s_knob, 23, 0);
    lv_obj_set_style_transform_zoom(s_knob, 256, 0);

    lv_obj_t *ch_ring = make_obj(s_knob);
    lv_obj_add_style(ch_ring, &st_border_texthi, 0);
    lv_obj_set_size(ch_ring, 16, 16);
    lv_obj_align(ch_ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ch_ring, 1, 0);

    lv_obj_t *ch_dot = make_obj(s_knob);
    lv_obj_add_style(ch_dot, &st_fill_texthi, 0);
    lv_obj_set_size(ch_dot, 4, 4);
    lv_obj_align(ch_dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ch_dot, LV_OPA_60, 0);
}

// Joystick panel — floating card in the bottom left corner, aligned with the
// camera bottom edge. Badge row on top, the stick below.
static void make_joy_panel(void)
{
    lv_obj_t *joy_panel = make_panel(PANEL_GAP,
                                     CAM_Y + CAM_H - PANEL_H,
                                     SIDE_W, PANEL_H);

    make_section_hdr(joy_panel, "JOYSTICK", HDR_Y);
    make_cmd_badges(joy_panel);
    make_joystick(joy_panel);
}

// Themes dropdown — floating card under the topbar's right side, hidden
// until THEMES is tapped. Each item is labelled in its theme's accent colour;
// a dot marks the active theme (toggled by scout_ui_set_theme()).
static void make_theme_menu(void)
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
        lv_obj_add_event_cb(item, theme_item_event, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);

        // Each row keeps its own theme's accent — a fixed colour, not the
        // current one — so it is set locally rather than via a shared style.
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

// Small accent tick centred on one edge of the viewfinder, sitting in the
// gap band outside the video region so the blit never paints over it.
static void make_cam_tick(int32_t x, int32_t y, bool horiz)
{
    lv_obj_t *t = make_obj(s_root);
    lv_obj_add_style(t, &st_fill_accent, 0);
    lv_obj_set_size(t, horiz ? 14 : 2, horiz ? 2 : 14);
    lv_obj_set_pos(t, x, y);
    lv_obj_set_style_radius(t, 1, 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_70, 0);
}

// Viewfinder corners — the video is blitted between them by render.c.
static void make_cam_corners(void)
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

    make_cam_tick(CAM_X + CAM_W / 2 - 7, CAM_Y - CAM_GAP + 1,        true);
    make_cam_tick(CAM_X + CAM_W / 2 - 7, CAM_Y + CAM_H + CAM_GAP - 3, true);
    make_cam_tick(CAM_X - CAM_GAP + 1,        CAM_Y + CAM_H / 2 - 7, false);
    make_cam_tick(CAM_X + CAM_W + CAM_GAP - 3, CAM_Y + CAM_H / 2 - 7, false);
}

// CONFIG panel — a single card filling the right side, hidden until the CONFIG
// topbar label is tapped. Holds the camera controls; rows map 1:1 to the
// camera-control commands the cam node accepts (cam_ctrl_cmd_t):
//   CAMERA     on/off switch        — CAM_CTRL_CAMERA_ON (0x01) / _OFF (0x02)
//   QUALITY    slider 0-63, def 20  — CAM_CTRL_SET_QUALITY    (0x10, lower = better JPEG)
//   BRIGHT     slider -2..2, def 0  — CAM_CTRL_SET_BRIGHTNESS (0x11)
//   CONTRAST   slider -2..2, def 0  — CAM_CTRL_SET_CONTRAST   (0x12)
//   SATURATION slider -2..2, def 0  — CAM_CTRL_SET_SATURATION (0x13)
// The controls are presentational for now; wire each to its command over
// CMD_PORT once the camera-control transport lands.
static void make_config_panel(void)
{
    s_cfg_panel = make_panel(SCREEN_W - PANEL_GAP - SIDE_W, CAM_Y, SIDE_W, CFG_PANEL_H);
    lv_obj_add_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);

    make_section_hdr(s_cfg_panel, "CAM CONFIG", HDR_Y);

    make_switch_row(s_cfg_panel, "CAMERA", 68, true);
    make_cfg_sep(s_cfg_panel, 116);
    make_slider_row(s_cfg_panel, "QUALITY",    "20",  0, 63, 20, 148, slider_num_event);
    make_cfg_sep(s_cfg_panel, 196);
    make_slider_row(s_cfg_panel, "BRIGHT",     " 0", -2,  2,  0, 228, slider_num_event);
    make_cfg_sep(s_cfg_panel, 276);
    make_slider_row(s_cfg_panel, "CONTRAST",   " 0", -2,  2,  0, 308, slider_num_event);
    make_cfg_sep(s_cfg_panel, 356);
    make_slider_row(s_cfg_panel, "SATURATION", " 0", -2,  2,  0, 388, slider_num_event);
    make_apply_btn(s_cfg_panel);

    if(s_config_open) lv_obj_clear_flag(s_cfg_panel, LV_OBJ_FLAG_HIDDEN);
}

// Opaque cover over the camera region with a centred message. Hidden by default;
// scene_render shows/hides it via scout_ui_overlay() (e.g. "WAITING FOR CAM").
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

    scout_ui_overlay(s_overlay_text);   // re-apply any pending message
}

// Builds the full UI under s_root. Called once at init; theme switches recolour
// the shared styles instead of rebuilding, so everything here is created a
// single time for the life of the program.
static void build_ui(void)
{
    make_stripe_tile();
    styles_init();
    styles_apply_theme();

    lv_obj_add_style(lv_scr_act(), &st_fill_bg, 0);

    s_root = make_obj(lv_scr_act());
    lv_obj_set_size(s_root, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(s_root, 0, 0);

    make_topbar();
    make_botbar();
    make_tele_panel();
    make_joy_panel();
    make_cam_corners();
    make_scene_overlay();
    make_theme_menu();
    make_config_panel();   // last so the config panel layers above everything when open
}

void scout_ui_init(void)
{
    build_ui();
}

// Theme switch — recolour the shared styles, re-bake the crosshatch tile, then
// ask LVGL to refresh. Widgets whose colours follow runtime state (link, wifi,
// command badges) are re-applied; the active-theme name and dropdown marker are
// updated directly. Nothing is destroyed or recreated.
void scout_ui_set_theme(uint8_t idx)
{
    idx %= THEME_COUNT;
    s_th = &s_themes[idx];

    styles_apply_theme();
    make_stripe_tile();                          // re-bake the crosshatch in the new line colour
    lv_img_cache_invalidate_src(&s_stripe_tile); // drop the cached decode of the old tile
    lv_obj_report_style_change(NULL);            // refresh every object using the shared styles

    scout_ui_update(s_wifi_level);               // link + wifi indicators
    update_cmd_badges(s_cmd);                     // command badge highlight

    lv_label_set_text(s_theme_name_lbl, s_th->name);
    for(uint8_t i = 0; i < THEME_COUNT; i++)
        lv_obj_add_flag(s_theme_dots[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_theme_dots[idx], LV_OBJ_FLAG_HIDDEN);

    lv_obj_invalidate(lv_scr_act());
}

void scout_ui_update(uint8_t wifi_level)
{
    s_wifi_level = wifi_level;
    if(wifi_level == 0)
        lv_obj_clear_flag(s_wifi_slash, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(s_wifi_slash, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_line_color(s_wifi_slash, lv_color_hex(COL_BAD), 0);
    lv_obj_set_style_bg_color(s_wifi_dot,
        lv_color_hex(wifi_level ? COL_TEXT_HI : COL_BAD), 0);
    for(int i = 0; i < 3; i++) {
        lv_obj_set_style_arc_color(s_wifi_arcs[i],
            lv_color_hex(wifi_level > (uint8_t)(i + 1) ? COL_TEXT_HI : COL_LINE),
            LV_PART_MAIN);
    }

    lv_label_set_text(s_link_lbl, wifi_level ? "LIVE" : "NO LINK");
    lv_obj_set_style_text_color(s_link_lbl,
        lv_color_hex(wifi_level ? COL_TEXT_MID : COL_TEXT_LO), 0);
    lv_obj_set_style_bg_color(s_link_dot,
        lv_color_hex(wifi_level ? COL_GOOD : COL_BAD), 0);
}

void scout_ui_update_telemetry(const cam_diag_pkt_t *d)
{
    s_cam_diag = *d;
    for(size_t i = 0; i < sizeof s_tele / sizeof s_tele[0]; i++) {
        tele_field_t *f = &s_tele[i];
        char buf[16];
        f->fmt(buf, sizeof buf, d);
        if(strcmp(buf, f->last) != 0) {
            lv_label_set_text(*f->widget, buf);
            memcpy(f->last, buf, sizeof f->last);
        }
    }
}

void scout_ui_overlay(const char *text)
{
    s_overlay_text = text;   // cached so make_scene_overlay can restore it at init
    if(text) {
        lv_label_set_text(s_scene_label, text);
        lv_obj_clear_flag(s_scene_overlay, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_scene_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

// A header/footer hairline label on the intro overlay — small, wide-tracked,
// sitting on the bare background to frame the hero like the top/bottom bars.
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

    // Overlay on the main screen — avoids lv_scr_load framebuffer issues
    s_intro_overlay = make_obj(lv_scr_act());
    lv_obj_add_style(s_intro_overlay, &st_fill_bg, 0);
    lv_obj_set_size(s_intro_overlay, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(s_intro_overlay, 0, 0);

    // Viewfinder brackets at the four corners — the signature framing motif.
    make_corner(s_intro_overlay, INTRO_FRAME, INTRO_FRAME,
                INTRO_CORNER, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_TOP, LV_OPA_50);
    make_corner(s_intro_overlay, SCREEN_W - INTRO_FRAME - INTRO_CORNER, INTRO_FRAME,
                INTRO_CORNER, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_TOP, LV_OPA_50);
    make_corner(s_intro_overlay, INTRO_FRAME, SCREEN_H - INTRO_FRAME - INTRO_CORNER,
                INTRO_CORNER, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, LV_OPA_50);
    make_corner(s_intro_overlay, SCREEN_W - INTRO_FRAME - INTRO_CORNER,
                SCREEN_H - INTRO_FRAME - INTRO_CORNER,
                INTRO_CORNER, LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM, LV_OPA_50);

    // Header hairline: maker brand left, boot tag right — vertically centred
    // on the corner brackets.
    intro_frame_label("LINTECH", &st_fg_lo,
                      LV_ALIGN_TOP_LEFT, INTRO_FRAME + 14, INTRO_TAG_DY);
    intro_frame_label("BOOT SEQUENCE", &st_fg_lo,
                      LV_ALIGN_TOP_RIGHT, -(INTRO_FRAME + 14), INTRO_TAG_DY);

    // Footer hairline: firmware version left, RTOS tag right (accent value),
    // matching the bottom bar's RTOS | FREERTOS cluster.
    intro_frame_label("FW v1.0.0", &st_fg_lo,
                      LV_ALIGN_BOTTOM_LEFT, INTRO_FRAME + 14, -INTRO_TAG_DY);
    intro_frame_label("FREERTOS", &st_fg_accent,
                      LV_ALIGN_BOTTOM_RIGHT, -(INTRO_FRAME + 14), -INTRO_TAG_DY);
    intro_frame_label("RTOS", &st_fg_lo,
                      LV_ALIGN_BOTTOM_RIGHT, -(INTRO_FRAME + 106), -INTRO_TAG_DY);

    // Hero wordmark.
    lv_obj_t *logo = make_label(s_intro_overlay, "SCOUT", &st_fg_accent, &st_font_logo);
    lv_obj_set_style_text_letter_space(logo, 8, 0);
    lv_obj_align(logo, LV_ALIGN_CENTER, 4, INTRO_LOGO_Y);   // +4 optical centre vs letter-space

    // Accent divider under the wordmark — a centred line fading out to each
    // side, built from two mirrored gradient halves. The gradient endpoints
    // are baked locally; the intro is built once at boot, before any switch.
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

    // Subtitle / product line.
    lv_obj_t *sub = make_label(s_intro_overlay, "DEVELOPED BY LINTECH", &st_fg_mid, NULL);
    lv_obj_set_style_text_letter_space(sub, 8, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, INTRO_SUB_Y);

    // Boot-step dots — one per init step, centred, lit as each step completes.
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
    // Accent glow so the fill reads as energised, not a flat block.
    lv_obj_set_style_shadow_color(s_intro_bar_fill, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_shadow_width(s_intro_bar_fill, 12, 0);
    lv_obj_set_style_shadow_spread(s_intro_bar_fill, 0, 0);
    lv_obj_set_style_shadow_opa(s_intro_bar_fill, LV_OPA_40, 0);

    // Status row: an accent activity dot, the current step left-aligned, and
    // the percentage right-aligned. Fixed widths + alignment keep the edges
    // anchored to the bar as the text changes.
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

    // Light the dot for the step that just completed.
    if(s_intro_step >= 1 && s_intro_step <= s_intro_total &&
       s_intro_dots[s_intro_step - 1]) {
        lv_obj_set_style_bg_color(s_intro_dots[s_intro_step - 1],
                                  lv_color_hex(COL_ACCENT), 0);
    }

    if(s_intro_step == s_intro_total) {
        lv_timer_t *t = lv_timer_create(intro_close_cb, INTRO_HOLD_MS, NULL);
        lv_timer_set_repeat_count(t, 1);
    }

    // Render directly — during boot the render loop is not running yet,
    // so this is what puts the step on screen while its init call blocks.
    lv_refr_now(lv_disp_get_default());
}

void scout_ui_get_joy(int16_t *x, int16_t *y) { *x = s_joy_x; *y = s_joy_y; }
