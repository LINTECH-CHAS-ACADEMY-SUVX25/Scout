#pragma once
#include <stdbool.h>

// Init the I2C bus + BME280 (SDA=GPIO2, SCL=GPIO3, addr 0x77).
// Returns true if the chip id check passed and the sensor is configured.
// On failure the cam runs without environmental data (bme280_read returns false).
bool bme280_init(void);

// Read compensated values: temp in °C, hum in %RH, pres in Pa.
// Returns false if the sensor isn't present/ready (outputs untouched).
bool bme280_read(float *temp, float *hum, float *pres);
