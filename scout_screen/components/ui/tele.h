#pragma once
#include "rc_protocol.h"

/** @brief Builds the telemetry panel with temperature, humidity and pressure rows. */
void tele_panel_build(void);

void scout_ui_update_telemetry(const cam_diag_pkt_t *d);
