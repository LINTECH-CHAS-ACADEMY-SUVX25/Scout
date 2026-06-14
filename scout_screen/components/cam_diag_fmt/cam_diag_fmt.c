#include "cam_diag_fmt.h"
#include <stdio.h>

void cam_diag_fmt_temp(char *out, size_t n, const cam_diag_pkt_t *d)
{
    int t     = d->temp_cdeg;          // 0.01 °C
    int whole = t / 100;
    int frac  = (t < 0 ? -t : t) % 100 / 10;
    const char *sign = (t < 0 && whole == 0) ? "-" : "";
    snprintf(out, n, "%s%d.%d C", sign, whole, frac);
}

void cam_diag_fmt_humi(char *out, size_t n, const cam_diag_pkt_t *d)
{
    snprintf(out, n, "%u %%", (unsigned)d->humidity_pct);
}

void cam_diag_fmt_pres(char *out, size_t n, const cam_diag_pkt_t *d)
{
    snprintf(out, n, "%lu hPa", (unsigned long)(d->pressure_pa / 100));
}
