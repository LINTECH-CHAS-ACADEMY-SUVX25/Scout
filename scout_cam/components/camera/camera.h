#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

// Configures the OV2640 sensor and allocates frame buffers in PSRAM.
// Must be called after WiFi is connected (camera and WiFi share DRAM).
// Returns the esp_camera_init result — the driver reports failures, the caller
// decides policy (retry, degrade, ...). Logs "camera ready" on success.
esp_err_t camera_init(void);

// Locks the next available JPEG frame. Returns false if the camera is not ready.
// Call camera_release() after the data has been sent.
bool camera_capture(const uint8_t **buf, size_t *len);

// Releases the frame buffer locked by camera_capture().
void camera_release(void);

// Applies a single OV2640 image setting via esp_camera_sensor_get().
// cmd is a CAM_CTRL_SET_* value from rc_protocol.h; value range depends on command.
// Returns false if the sensor is unavailable or the command is unrecognised.
bool camera_apply_setting(uint8_t cmd, int8_t value);
