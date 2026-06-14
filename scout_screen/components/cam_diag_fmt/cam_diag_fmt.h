#pragma once
#include "rc_protocol.h"
#include <stddef.h>

// Format a cam_diag sensor field into out (n bytes, NUL-terminated). Integer-only.
void cam_diag_fmt_temp(char *out, size_t n, const cam_diag_pkt_t *d);  // "27.1 C"
void cam_diag_fmt_humi(char *out, size_t n, const cam_diag_pkt_t *d);  // "36 %"
void cam_diag_fmt_pres(char *out, size_t n, const cam_diag_pkt_t *d);  // "1002 hPa"
