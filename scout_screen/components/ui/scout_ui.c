#include "scout_ui.h"
#include "display.h"
#include "rc_protocol.h"
#include "lvgl.h"
#include <math.h>
#include <stdbool.h>

// Owns the full UI layout — widget creation, event callbacks, and the intro
// overlay. Compiled by both the scout_screen firmware and the PC simulator
// (sim/), so it must stay free of ESP-IDF and FreeRTOS headers. The simulator
// provides its own display.h with SCREEN_W/SCREEN_H.

// Max knob offset from centre — base half (65) minus halo half (31), so
// neither the knob nor its halo ever leaves the 130px joystick frame.
#define JOY_RADIUS 34

// Intro overlay loading bar — slim rounded track with a gradient fill.
// The fill sits inside the track's 1px border + 2px padding (3px inset/side).
#define INTRO_BAR_W   440
#define INTRO_BAR_H   14
#define INTRO_FILL_W  (INTRO_BAR_W - 6)
#define INTRO_FILL_H  (INTRO_BAR_H - 6)
#define INTRO_HOLD_MS 1200   // how long the finished bar stays before the overlay closes

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

// Palette — dark tech / premium
#define COL_BG         0x0A0E14
#define COL_BAR        0x0D1117
#define COL_PANEL      0x141A23
#define COL_LINE       0x232B36
#define COL_ACCENT     0x22D3EE
#define COL_ACCENT_DEEP 0x0891B2   // gradient start for the loading fill
#define COL_TEXT_HI    0xE6EDF3
#define COL_TEXT_MID   0x9BA7B4
#define COL_TEXT_LO    0x5C6773
#define COL_GOOD       0x34D399
#define COL_BAD        0xF87171
#define COL_BADGE_BG   0x1B2230
#define COL_BADGE_ON   0x10222A

#define XSTR(x) STR(x)
#define STR(x)  #x

// UI font — Press Start 2P (generated C fonts, see the respective .c files).
// The 96px logo font only contains space + A-Z to keep its size down.
LV_FONT_DECLARE(press_start_2p_8);
LV_FONT_DECLARE(press_start_2p_96);
#define UI_FONT   (&press_start_2p_8)
#define LOGO_FONT (&press_start_2p_96)   // intro logo

static volatile uint8_t s_cmd = CMD_STOP; 

// Widget handles

static lv_obj_t *s_intro_overlay;
static lv_obj_t *s_intro_bar_fill;
static lv_obj_t *s_intro_status;
static lv_obj_t *s_intro_pct;
static uint8_t   s_intro_total;
static uint8_t   s_intro_step;
static lv_obj_t *s_knob;
static lv_obj_t *s_halo;
static lv_obj_t *s_wifi_dot;
static lv_obj_t *s_wifi_arcs[3];
static lv_obj_t *s_cmd_badges[5];
static lv_obj_t *s_val_temp;
static lv_obj_t *s_val_humi;
static lv_obj_t *s_val_pres;

// Widget helpers

static lv_obj_t *make_obj(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                              lv_color_t color, const lv_font_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
    return l;
}

// Stripe tile pixel data — RGB565 + alpha, filled in by make_stripe_tile().
static uint8_t      s_stripe_map[STRIPE_TILE * STRIPE_TILE * 3];
static lv_img_dsc_t s_stripe_tile;

// Builds the crosshatch tile at init: (x + y) wrapping inside STRIPE_W gives
// the "/" diagonals, (x - y) the "\" ones; everything else stays transparent.
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

// Lays the stripe texture over a card background.
static void add_stripes(lv_obj_t *card)
{
    lv_obj_set_style_bg_img_src(card, &s_stripe_tile, 0);
    lv_obj_set_style_bg_img_tiled(card, true, 0);
}

// Thin vertical hairline used to separate groups in the top and bottom bars.
static void make_vsep(lv_obj_t *parent, lv_align_t align, int32_t x)
{
    lv_obj_t *s = make_obj(parent);
    lv_obj_set_size(s, 1, 12);
    lv_obj_align(s, align, x, 0);
    lv_obj_set_style_bg_color(s, lv_color_hex(COL_LINE), 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
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

// Accent corner bracket framing the camera area like a viewfinder.
static void make_cam_corner(int32_t x, int32_t y, lv_border_side_t side)
{
    lv_obj_t *c = make_obj(lv_scr_act());
    lv_obj_set_size(c, CAM_CORNER, CAM_CORNER);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_border_width(c, 2, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_border_side(c, side, 0);
    lv_obj_set_style_border_opa(c, LV_OPA_COVER, 0);
}

// Floating rounded panel — the shared card style for the corner widgets.
static lv_obj_t *make_panel(int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *p = make_obj(lv_scr_act());
    lv_obj_set_size(p, w, h);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_style_radius(p, 8, 0);
    lv_obj_set_style_bg_color(p, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(p, 1, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(COL_LINE), 0);
    add_stripes(p);
    return p;
}

// Section header with a cyan accent mark on the left.
static void make_section_hdr(lv_obj_t *parent, const char *text, int32_t y)
{
    // y-3 centres the 14px mark on the 8px text's midline (y+4)
    lv_obj_t *mark = make_obj(parent);
    lv_obj_set_size(mark, 3, 14);
    lv_obj_set_pos(mark, PAD, y - 3);
    lv_obj_set_style_radius(mark, 1, 0);
    lv_obj_set_style_bg_color(mark, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_opa(mark, LV_OPA_COVER, 0);

    lv_obj_t *l = make_label(parent, text,
        lv_color_hex(COL_TEXT_MID), UI_FONT);
    lv_obj_set_style_text_letter_space(l, 2, 0);
    lv_obj_set_pos(l, PAD + 10, y);
}

static lv_obj_t *create_joy_tick(lv_obj_t *parent, lv_align_t align,
                                   int32_t xo, int32_t yo, bool horiz)
{
    lv_obj_t *t = lv_obj_create(parent);
    lv_obj_set_size(t, horiz ? 7 : 1, horiz ? 1 : 7);
    lv_obj_align(t, align, xo, yo);
    lv_obj_set_style_bg_color(t, lv_color_hex(COL_TEXT_LO), 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(t, 1, 0);
    lv_obj_set_style_border_width(t, 0, 0);
    lv_obj_clear_flag(t, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return t;
}

static lv_obj_t *make_tele_row(lv_obj_t *parent, const char *key,
                                 const char *init_val, lv_color_t val_color,
                                 int32_t y)
{
    lv_obj_t *row = make_obj(parent);
    lv_obj_set_size(row, TELE_ROW_W, 22);
    lv_obj_set_pos(row, 0, y);

    lv_obj_t *k = make_label(row, key,
        lv_color_hex(COL_TEXT_MID), UI_FONT);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *v = make_label(row, init_val, val_color, UI_FONT);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, 0, 0);
    return v;
}

// Command badges

static void update_cmd_badges(uint8_t cmd)
{
    static const uint8_t masks[5] = {
        CMD_FORWARD, CMD_BACKWARD, 0xFF, CMD_LEFT, CMD_RIGHT
    };
    for(int i = 0; i < 5; i++) {
        bool active = (i == 2) ? (cmd == CMD_STOP) : (cmd & masks[i]);
        lv_obj_set_style_bg_color(s_cmd_badges[i],
            lv_color_hex(active ? COL_BADGE_ON : COL_BADGE_BG), 0);
        lv_obj_set_style_border_color(s_cmd_badges[i],
            lv_color_hex(active ? COL_ACCENT : COL_LINE), 0);

        lv_obj_t *lbl = lv_obj_get_child(s_cmd_badges[i], 0);
        if(lbl) {
            lv_obj_set_style_text_color(lbl,
                lv_color_hex(active ? COL_ACCENT : COL_TEXT_LO), 0);
        }
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

        uint8_t cmd = CMD_STOP;
        if(dy < -15) cmd |= CMD_FORWARD;
        if(dy >  15) cmd |= CMD_BACKWARD;
        if(dx < -15) cmd |= CMD_LEFT;
        if(dx >  15) cmd |= CMD_RIGHT;
        s_cmd = cmd;
        update_cmd_badges(s_cmd);
    } else {
        lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_transform_zoom(s_knob, 256, 0);
        lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A434F), 0);
        s_cmd = CMD_STOP;
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

// UI assembly — one builder per visible module, called from scout_ui_init

// Topbar — floating card with the brand and the WiFi signal symbol.
static void make_topbar(void)
{
    lv_obj_t *topbar = make_obj(lv_scr_act());
    lv_obj_set_size(topbar, SCREEN_W - 2 * PANEL_GAP, BAR_H);
    lv_obj_align(topbar, LV_ALIGN_TOP_MID, 0, PANEL_GAP);
    lv_obj_set_style_radius(topbar, 8, 0);
    lv_obj_set_style_bg_color(topbar, lv_color_hex(COL_BAR), 0);
    lv_obj_set_style_bg_opa(topbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(topbar, 1, 0);
    lv_obj_set_style_border_color(topbar, lv_color_hex(COL_LINE), 0);

    lv_obj_t *logo = make_label(topbar, "SCOUT",
        lv_color_hex(COL_ACCENT), UI_FONT);
    lv_obj_set_style_text_letter_space(logo, 6, 0);
    lv_obj_align(logo, LV_ALIGN_LEFT_MID, 14, 0);

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
}

// Bottombar — floating card with the network facts and the RTOS tag.
static void make_botbar(void)
{
    lv_obj_t *botbar = make_obj(lv_scr_act());
    lv_obj_set_size(botbar, SCREEN_W - 2 * PANEL_GAP, BAR_H);
    lv_obj_align(botbar, LV_ALIGN_BOTTOM_MID, 0, -PANEL_GAP);
    lv_obj_set_style_radius(botbar, 8, 0);
    lv_obj_set_style_bg_color(botbar, lv_color_hex(COL_BAR), 0);
    lv_obj_set_style_bg_opa(botbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(botbar, 1, 0);
    lv_obj_set_style_border_color(botbar, lv_color_hex(COL_LINE), 0);

    const char *bot_keys[] = { "IP", "UDP" };
    const char *bot_vals[] = { S3_IP, XSTR(VID_PORT) };
    const int32_t key_w[]  = { 24, 38 };
    int32_t bot_x = 14;
    for(int i = 0; i < 2; i++) {
        lv_obj_t *k = make_label(botbar, bot_keys[i],
            lv_color_hex(COL_TEXT_LO), UI_FONT);
        lv_obj_align(k, LV_ALIGN_LEFT_MID, bot_x, 0);
        bot_x += key_w[i];
        lv_obj_t *v = make_label(botbar, bot_vals[i],
            lv_color_hex(COL_TEXT_MID), UI_FONT);
        lv_obj_align(v, LV_ALIGN_LEFT_MID, bot_x, 0);
        bot_x += (i == 0) ? 110 : 50;
    }
    make_vsep(botbar, LV_ALIGN_LEFT_MID, 140);
    make_vsep(botbar, LV_ALIGN_RIGHT_MID, -165);

    lv_obj_t *rtos_k = make_label(botbar, "RTOS",
        lv_color_hex(COL_TEXT_LO), UI_FONT);
    lv_obj_align(rtos_k, LV_ALIGN_RIGHT_MID, -120, 0);
    lv_obj_t *rtos_v = make_label(botbar, "FREERTOS",
        lv_color_hex(COL_ACCENT), UI_FONT);
    lv_obj_align(rtos_v, LV_ALIGN_RIGHT_MID, -14, 0);
}

// Telemetry panel — floating card in the top right corner, aligned with the
// camera top edge. Holds the value rows in a raised inner card.
static void make_tele_panel(void)
{
    lv_obj_t *tele_panel = make_panel(SCREEN_W - PANEL_GAP - SIDE_W,
                                      CAM_Y,
                                      SIDE_W, PANEL_H);

    make_section_hdr(tele_panel, "TELEMETRI", HDR_Y);

    // Raised card around the telemetry rows
    lv_obj_t *tele_card = lv_obj_create(tele_panel);
    lv_obj_set_size(tele_card, ROW_W, TELE_CARD_H);
    lv_obj_set_pos(tele_card, PAD, TELE_CARD_Y);
    lv_obj_set_style_radius(tele_card, 8, 0);
    lv_obj_set_style_bg_color(tele_card, lv_color_hex(COL_BAR), 0);
    lv_obj_set_style_bg_opa(tele_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tele_card, 1, 0);
    lv_obj_set_style_border_color(tele_card, lv_color_hex(COL_LINE), 0);
    lv_obj_set_style_pad_all(tele_card, 0, 0);
    lv_obj_clear_flag(tele_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *tele_cont = make_obj(tele_card);
    lv_obj_set_size(tele_cont, TELE_ROW_W, 2 * TELE_PITCH + 22);
    lv_obj_set_pos(tele_cont, CARD_PAD, CARD_PAD);

    s_val_temp = make_tele_row(tele_cont, "temperature", "--.- C",   lv_color_hex(COL_ACCENT), 0 * TELE_PITCH);
    s_val_humi = make_tele_row(tele_cont, "humidity",    "-- %",     lv_color_hex(COL_ACCENT), 1 * TELE_PITCH);
    s_val_pres = make_tele_row(tele_cont, "pressure",    "---- hPa", lv_color_hex(COL_ACCENT), 2 * TELE_PITCH);

    for(int i = 1; i < 3; i++) {
        lv_obj_t *sep = make_obj(tele_cont);
        lv_obj_set_size(sep, TELE_ROW_W, 1);
        lv_obj_set_pos(sep, 0, i * TELE_PITCH - 4);
        lv_obj_set_style_bg_color(sep, lv_color_hex(COL_LINE), 0);
        lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    }
}

// Command badge row (FWD/BWD/STP/LFT/RGT) at the top of the joystick panel.
static void make_cmd_badges(lv_obj_t *joy_panel)
{
    const char *badge_labels[5] = { "FWD", "BWD", "STP", "LFT", "RGT" };
    int32_t badge_x = (SIDE_W - (5 * BADGE_W + 4 * BADGE_GAP)) / 2;   // centre the row
    for(int i = 0; i < 5; i++) {
        lv_obj_t *badge = lv_obj_create(joy_panel);
        lv_obj_set_size(badge, BADGE_W, BADGE_H);
        lv_obj_set_pos(badge, badge_x, 44);
        lv_obj_set_style_radius(badge, BADGE_H / 2, 0);
        lv_obj_set_style_bg_color(badge, lv_color_hex(COL_BADGE_BG), 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(badge, 1, 0);
        lv_obj_set_style_border_color(badge, lv_color_hex(COL_LINE), 0);
        lv_obj_set_style_pad_all(badge, 0, 0);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *bl = make_label(badge, badge_labels[i],
            lv_color_hex(COL_TEXT_LO), UI_FONT);
        lv_obj_align(bl, LV_ALIGN_CENTER, 0, 0);
        s_cmd_badges[i] = badge;
        badge_x += BADGE_STEP;
    }
    update_cmd_badges(CMD_STOP);
}

// The joystick itself — concentric rings, touch base, halo and knob.
static void make_joystick(lv_obj_t *joy_panel)
{
    // Outer concentric ring gives the joystick an instrument-dial look
    lv_obj_t *joy_ring = make_obj(joy_panel);
    lv_obj_set_size(joy_ring, 146, 146);
    lv_obj_set_pos(joy_ring, (SIDE_W - 146) / 2, 78);
    lv_obj_set_style_radius(joy_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(joy_ring, 1, 0);
    lv_obj_set_style_border_color(joy_ring, lv_color_hex(COL_LINE), 0);
    lv_obj_set_style_border_opa(joy_ring, LV_OPA_COVER, 0);

    lv_obj_t *base = lv_obj_create(joy_panel);
    lv_obj_set_size(base, 130, 130);
    lv_obj_set_pos(base, (SIDE_W - 130) / 2, 86);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(base, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(base, 2, 0);
    lv_obj_set_style_border_color(base, lv_color_hex(COL_LINE), 0);
    lv_obj_set_style_border_opa(base, LV_OPA_COVER, 0);
    // Shadow removed — blurred shadow rendering into PSRAM framebuffers
    // stalled the LVGL render pass and tripped the task watchdog.
    lv_obj_clear_flag(base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESSING,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_RELEASED,   NULL);
    lv_obj_add_event_cb(base, joy_event, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *guide = lv_obj_create(base);
    lv_obj_set_size(guide, 82, 82);
    lv_obj_align(guide, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(guide, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(guide, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(guide, 1, 0);
    lv_obj_set_style_border_color(guide, lv_color_hex(COL_LINE), 0);
    lv_obj_set_style_border_opa(guide, LV_OPA_COVER, 0);
    lv_obj_clear_flag(guide, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    create_joy_tick(base, LV_ALIGN_TOP_MID,    0,  7, false);
    create_joy_tick(base, LV_ALIGN_BOTTOM_MID, 0, -7, false);
    create_joy_tick(base, LV_ALIGN_LEFT_MID,   7,  0, true);
    create_joy_tick(base, LV_ALIGN_RIGHT_MID, -7,  0, true);

    s_halo = lv_obj_create(base);
    lv_obj_set_size(s_halo, 62, 62);
    lv_obj_align(s_halo, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_halo, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_opa(s_halo, LV_OPA_20, 0);
    lv_obj_set_style_border_width(s_halo, 0, 0);
    lv_obj_clear_flag(s_halo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    s_knob = lv_obj_create(base);
    lv_obj_set_size(s_knob, 46, 46);
    lv_obj_align(s_knob, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_knob, lv_color_hex(0x3A434F), 0);
    lv_obj_set_style_bg_opa(s_knob, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_knob, 0, 0);
    // Shadow removed — same watchdog/stall reason as the base above.
    lv_obj_set_style_transform_pivot_x(s_knob, 23, 0);
    lv_obj_set_style_transform_pivot_y(s_knob, 23, 0);
    lv_obj_set_style_transform_zoom(s_knob, 256, 0);
    lv_obj_clear_flag(s_knob, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ch_ring = lv_obj_create(s_knob);
    lv_obj_set_size(ch_ring, 16, 16);
    lv_obj_align(ch_ring, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ch_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ch_ring, 1, 0);
    lv_obj_set_style_border_color(ch_ring, lv_color_hex(COL_TEXT_HI), 0);
    lv_obj_set_style_border_opa(ch_ring, LV_OPA_40, 0);
    lv_obj_clear_flag(ch_ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ch_dot = lv_obj_create(s_knob);
    lv_obj_set_size(ch_dot, 4, 4);
    lv_obj_align(ch_dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ch_dot, lv_color_hex(COL_TEXT_HI), 0);
    lv_obj_set_style_bg_opa(ch_dot, LV_OPA_60, 0);
    lv_obj_set_style_border_width(ch_dot, 0, 0);
    lv_obj_clear_flag(ch_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
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

// Viewfinder corners — the video is blitted between them by render.c.
static void make_cam_corners(void)
{
    make_cam_corner(CAM_X - CAM_GAP, CAM_Y - CAM_GAP,
                    LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT);
    make_cam_corner(CAM_X + CAM_W + CAM_GAP - CAM_CORNER, CAM_Y - CAM_GAP,
                    LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT);
    make_cam_corner(CAM_X - CAM_GAP, CAM_Y + CAM_H + CAM_GAP - CAM_CORNER,
                    LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_LEFT);
    make_cam_corner(CAM_X + CAM_W + CAM_GAP - CAM_CORNER,
                    CAM_Y + CAM_H + CAM_GAP - CAM_CORNER,
                    LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT);
}

void scout_ui_init(void)
{
    make_stripe_tile();

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    make_topbar();
    make_botbar();
    make_tele_panel();
    make_joy_panel();
    make_cam_corners();
}

// Future: scout_ui_update(float temp, float humi, float pres, uint8_t wifi_level)
// when cam sends sensor data
void scout_ui_update(uint8_t wifi_level)
{
    lv_obj_set_style_bg_color(s_wifi_dot,
        lv_color_hex(wifi_level ? COL_TEXT_HI : COL_BAD), 0);
    for(int i = 0; i < 3; i++) {
        lv_obj_set_style_arc_color(s_wifi_arcs[i],
            lv_color_hex(wifi_level > (uint8_t)i ? COL_TEXT_HI : COL_LINE),
            LV_PART_MAIN);
    }
}

void scout_ui_intro_screen(uint8_t total_steps)
{
    s_intro_total = total_steps ? total_steps : 1;
    s_intro_step  = 0;

    // Overlay on the main screen — avoids lv_scr_load framebuffer issues
    s_intro_overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_intro_overlay, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(s_intro_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_intro_overlay, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(s_intro_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_intro_overlay, 0, 0);
    lv_obj_set_style_radius(s_intro_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_intro_overlay, 0, 0);
    lv_obj_clear_flag(s_intro_overlay, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *logo = lv_label_create(s_intro_overlay);
    lv_label_set_text(logo, "SCOUT");
    lv_obj_set_style_text_color(logo, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_font(logo, LOGO_FONT, 0);
    lv_obj_set_style_text_letter_space(logo, 6, 0);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, -60);

    lv_obj_t *track = lv_obj_create(s_intro_overlay);
    lv_obj_set_size(track, INTRO_BAR_W, INTRO_BAR_H);
    lv_obj_align(track, LV_ALIGN_CENTER, 0, 44);
    lv_obj_set_style_bg_color(track, lv_color_hex(COL_PANEL), 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(track, 1, 0);
    lv_obj_set_style_border_color(track, lv_color_hex(COL_LINE), 0);
    lv_obj_set_style_border_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(track, INTRO_BAR_H / 2, 0);
    lv_obj_set_style_pad_all(track, 2, 0);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    s_intro_bar_fill = lv_obj_create(track);
    lv_obj_set_size(s_intro_bar_fill, 0, INTRO_FILL_H);
    lv_obj_set_pos(s_intro_bar_fill, 0, 0);
    lv_obj_set_style_radius(s_intro_bar_fill, INTRO_FILL_H / 2, 0);
    lv_obj_set_style_bg_color(s_intro_bar_fill, lv_color_hex(COL_ACCENT_DEEP), 0);
    lv_obj_set_style_bg_grad_color(s_intro_bar_fill, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_bg_grad_dir(s_intro_bar_fill, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(s_intro_bar_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_intro_bar_fill, 0, 0);
    lv_obj_set_style_pad_all(s_intro_bar_fill, 0, 0);
    lv_obj_clear_flag(s_intro_bar_fill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // Status left-aligned and percentage right-aligned under the bar. Both
    // labels get a fixed width and text alignment so changing text keeps the
    // edges anchored without re-aligning.
    s_intro_status = lv_label_create(s_intro_overlay);
    lv_label_set_text(s_intro_status, "STARTING");
    lv_obj_set_style_text_color(s_intro_status, lv_color_hex(COL_TEXT_MID), 0);
    lv_obj_set_style_text_font(s_intro_status, UI_FONT, 0);
    lv_obj_set_style_text_letter_space(s_intro_status, 4, 0);
    lv_obj_set_width(s_intro_status, 240);
    lv_obj_set_style_text_align(s_intro_status, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(s_intro_status, LV_ALIGN_CENTER, -(INTRO_BAR_W / 2) + 122, 72);

    s_intro_pct = lv_label_create(s_intro_overlay);
    lv_label_set_text(s_intro_pct, "0%");
    lv_obj_set_style_text_color(s_intro_pct, lv_color_hex(COL_TEXT_HI), 0);
    lv_obj_set_style_text_font(s_intro_pct, UI_FONT, 0);
    lv_obj_set_style_text_letter_space(s_intro_pct, 2, 0);
    lv_obj_set_width(s_intro_pct, 80);
    lv_obj_set_style_text_align(s_intro_pct, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(s_intro_pct, LV_ALIGN_CENTER, (INTRO_BAR_W / 2) - 40, 72);
}

void scout_ui_intro_step(const char *label)
{
    if(s_intro_overlay == NULL) return;

    if(s_intro_step < s_intro_total) s_intro_step++;
    lv_label_set_text(s_intro_status, label);
    lv_label_set_text_fmt(s_intro_pct, "%d%%", 100 * s_intro_step / s_intro_total);
    lv_obj_set_width(s_intro_bar_fill, INTRO_FILL_W * s_intro_step / s_intro_total);

    if(s_intro_step == s_intro_total) {
        lv_timer_t *t = lv_timer_create(intro_close_cb, INTRO_HOLD_MS, NULL);
        lv_timer_set_repeat_count(t, 1);
    }

    // Render directly — during boot the render loop is not running yet,
    // so this is what puts the step on screen while its init call blocks.
    lv_refr_now(lv_disp_get_default());
}

uint8_t scout_ui_get_cmd(void) { return s_cmd; }
