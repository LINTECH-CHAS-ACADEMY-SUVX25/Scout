#include "ui.h"
#include "rc_protocol.h"
#include "lvgl.h"

static uint8_t s_cmd = CMD_STOP;

uint8_t ui_get_cmd(void)
{
    return s_cmd;
}

static void btn_cb(lv_event_t *e)
{
    uint8_t mask = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_PRESSED)
        s_cmd |= mask;
    else
        s_cmd &= ~mask;
}

void ui_init(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x966FDE), 0);

    const struct { const char *sym; uint8_t mask; int ox, oy; } dpad[] = {
        { LV_SYMBOL_UP,    CMD_FORWARD,    0, -130 },
        { LV_SYMBOL_DOWN,  CMD_BACKWARD,   0,  -10 },
        { LV_SYMBOL_LEFT,  CMD_LEFT,     -70,  -70 },
        { LV_SYMBOL_RIGHT, CMD_RIGHT,     70,  -70 },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(lv_scr_act());
        lv_obj_set_size(btn, 60, 60);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, dpad[i].ox, dpad[i].oy);
        lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_PRESSED,  (void *)(uintptr_t)dpad[i].mask); // hold to drive
        lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_RELEASED, (void *)(uintptr_t)dpad[i].mask); // release to stop
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, dpad[i].sym);
        lv_obj_center(lbl);
    }
}
