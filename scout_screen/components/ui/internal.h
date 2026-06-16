#pragma once
// Shared declarations for UI component files only.
// Do not include this header from outside the ui/ component.

#include "lvgl.h"
#include "display.h"
#include "rc_protocol.h"
#include "themes.h"
#include "layout.h"
#include <stdbool.h>
#include <stdint.h>

// Root container, owned by scout_ui.c
extern lv_obj_t *s_root;

// Shared widget helpers, defined in scout_ui.c
lv_obj_t *make_obj(lv_obj_t *parent);
lv_obj_t *make_label(lv_obj_t *parent, const char *text, lv_style_t *fg, lv_style_t *font);
void      make_vsep(lv_obj_t *parent, lv_align_t align, int32_t x);
lv_obj_t *make_corner(lv_obj_t *parent, int32_t x, int32_t y, int32_t size, lv_border_side_t side, lv_opa_t opa);
lv_obj_t *make_panel(int32_t x, int32_t y, int32_t w, int32_t h);
void      make_section_hdr(lv_obj_t *parent, const char *text, int32_t y);
