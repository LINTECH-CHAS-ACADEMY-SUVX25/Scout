#pragma once
#include "esp_err.h"
#include "driver/i2c.h"


struct bme280_calib_t
{
	uint16_t T1 = 0;
	int16_t T2 = 0, T3 = 0;
	uint16_t P1 = 0;
	int16_t P2 = 0, P3 = 0, P4 = 0, P5 = 0, P6 = 0, P7 = 0, P8 = 0, P9 = 0;
	uint8_t H1 = 0;
	int16_t H2 = 0;
	uint8_t H3 = 0;
	int16_t H4 = 0, H5 = 0;
	int8_t H6 = 0;
};

struct raw_measurement_t
{
	int32_t adc_T = 0;
	int32_t adc_P = 0;
	int32_t adc_H = 0;
};

struct sensorReading
{
	int32_t T;
	uint32_t P;
	uint32_t H;
};

class bme280_sensor
{
public:

	bme280_sensor(i2c_port_t port, uint8_t address) : port_(port), address_(address) {}
	esp_err_t init();
	esp_err_t read_calibration();
	esp_err_t start_measurement();
	esp_err_t read_measurement(sensorReading &reading);

private: 

	esp_err_t write(uint8_t reg, uint8_t value);
	esp_err_t read(uint8_t reg, uint8_t *data, size_t len);
	esp_err_t read_raw_meas(raw_measurement_t *raw);
	int32_t compensate_T(int32_t adc_T);
	uint32_t compensate_P(int32_t adc_P);
	uint32_t compensate_H(int32_t adc_H);

	i2c_port_t port_;
	uint8_t address_ = 0;
	bme280_calib_t calib_;
	int32_t t_fine_ = 0;
};
