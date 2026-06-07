#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

// Configures the OV2640 sensor and allocates frame buffers in PSRAM.
// Must be called after WiFi is connected (camera and WiFi share DRAM).
esp_err_t camera_init(void);

// Locks the next available JPEG frame. Returns false if the camera is not ready.
// Call camera_release() after the data has been sent.
bool camera_capture(const uint8_t **buf, size_t *len);

// Releases the frame buffer locked by camera_capture().
void camera_release(void);
