#pragma once
#include <stdbool.h>

/**
 * @brief Inits the I2C bus and BME280 (SDA=GPIO2, SCL=GPIO3, addr 0x77).
 * @return false if the chip id check fails; the cam runs without environmental data.
 */
bool bme280_init(void);

/**
 * @brief Reads compensated values: temp in °C, hum in %RH, pres in Pa.
 * @return false if the sensor is not present/ready; outputs are left untouched.
 */
bool bme280_read(float *temp, float *hum, float *pres);
