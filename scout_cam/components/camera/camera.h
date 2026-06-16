#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Configures the OV2640 sensor and allocates frame buffers in PSRAM.
 *        Must be called after WiFi is connected (camera and WiFi share DRAM).
 */
esp_err_t camera_init(void);

/**
 * @brief Locks the next available JPEG frame. Must be followed by camera_release().
 * @return false if the camera is not ready.
 */
bool camera_capture(const uint8_t **buf, size_t *len);

void camera_release(void);

/**
 * @brief Applies a single OV2640 image setting via esp_camera_sensor_get().
 * @param cmd CAM_CTRL_SET_* value from rc_protocol.h.
 * @return false if the sensor is unavailable or the command is unrecognised.
 */
bool camera_apply_setting(uint8_t cmd, int8_t value);
