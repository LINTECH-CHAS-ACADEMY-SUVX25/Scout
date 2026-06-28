#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inits the BME280 over I2C and spawns the telemetry task, which reads
 *        the sensor every 2 s. A missing sensor is not fatal — telemetry_read()
 *        just keeps returning false.
 */
void telemetry_init(void);

/**
 * @brief Latest compensated reading: temp in °C, hum in %RH, pres in Pa.
 * @return false until the first successful read; outputs are left untouched.
 */
bool telemetry_read(float *temp_c, float *hum_pct, float *pres_pa);

#ifdef __cplusplus
}
#endif
