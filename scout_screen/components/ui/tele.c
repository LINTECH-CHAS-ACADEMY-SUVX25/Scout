#include "internal.h"
#include "tele.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_val_temp;
static lv_obj_t *s_val_humi;
static lv_obj_t *s_val_pres;

static cam_diag_pkt_t s_cam_diag;

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

static lv_obj_t *make_tele_row(lv_obj_t *parent, const char *key,
                                const char *init_val, int32_t y)
{
    lv_obj_t *row = make_obj(parent);
    lv_obj_set_size(row, TELE_ROW_W, 22);
    lv_obj_set_pos(row, 0, y);

    lv_obj_t *k = make_label(row, key, &st_fg_mid, NULL);
    lv_obj_align(k, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *chip = make_obj(row);
    lv_obj_add_style(chip, &st_chip, 0);
    lv_obj_set_size(chip, 84, 20);
    lv_obj_align(chip, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *v = make_label(chip, init_val, &st_fg_accent, NULL);
    lv_obj_align(v, LV_ALIGN_RIGHT_MID, -8, 0);
    return v;
}

void tele_panel_build(void)
{
    lv_obj_t *tele_panel = make_panel(PANEL_GAP, CAM_Y, SIDE_W, PANEL_H);

    make_section_hdr(tele_panel, "TELEMETRI", HDR_Y);

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
