#pragma once

// Layout constants shared across all UI component files.

#define BAR_H       36
#define PANEL_GAP   8
#define CONTENT_Y   (BAR_H + PANEL_GAP)
#define CONTENT_H   (SCREEN_H - 2 * CONTENT_Y)
#define SIDE_W      240
#define PAD         14
#define ROW_W       (SIDE_W - 2 * PAD)
#define HDR_Y       12
#define TELE_CARD_Y 78
#define TELE_PITCH  30

#define BADGE_W     38
#define BADGE_H     20
#define BADGE_GAP   7
#define BADGE_STEP  (BADGE_W + BADGE_GAP)

#define MENU_W      150
#define MENU_ITEM_H 34
#define MENU_PAD    6

#define CARD_PAD    12
#define TELE_ROW_W  (ROW_W - 2 * CARD_PAD - 2)
#define TELE_CARD_H (2 * TELE_PITCH + 22 + 2 * (CARD_PAD + 1))

#define PANEL_H 236

#define CAM_X       ((SCREEN_W - CAM_W) / 2)
#define CAM_Y       ((SCREEN_H - CAM_H) / 2)
#define CAM_GAP     8
#define CAM_CORNER  22

// Idle knob colour — fixed slate grey, same across all themes
#define KNOB_IDLE   0x3A434F

#define XSTR(x) STR(x)
#define STR(x)  #x
