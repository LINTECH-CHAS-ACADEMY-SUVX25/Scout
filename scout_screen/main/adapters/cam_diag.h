#pragma once
#include "rc_protocol.h"

// Latest diagnostics packet received from scout_cam over DIAG_PORT.
// Zero-initialised until the first packet arrives.
extern cam_diag_pkt_t cam_diag_latest;

// Starts the background task that listens on DIAG_PORT and updates cam_diag_latest.
void cam_diag_init(void);
