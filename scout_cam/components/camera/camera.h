#pragma once
#include "esp_err.h"

// Configures the OV2640 sensor and allocates frame buffers in PSRAM.
// Must be called after WiFi is connected (camera and WiFi share DRAM).
esp_err_t camera_init(void);
