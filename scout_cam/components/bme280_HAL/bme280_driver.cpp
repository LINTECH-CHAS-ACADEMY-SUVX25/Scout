#include "bme280_driver.hpp"
#include "esp_log.h"

static const char *TAG = "BME280";

#define I2C_SDA GPIO_NUM_2
#define I2C_SCL GPIO_NUM_3

#define I2C_FREQ_HZ 100000

esp_err_t bme280_sensor::init()
{	
	i2c_config_t conf = {};
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_SDA;
	conf.scl_io_num = I2C_SCL;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C_FREQ_HZ;
	conf.clk_flags = 0;
	
	esp_err_t err = i2c_param_config(port_, &conf);
	if(err != ESP_OK) return err;
	
	return i2c_driver_install(port_, conf.mode, 0, 0, 0);
}

esp_err_t bme280_sensor::write(uint8_t reg, uint8_t value)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address_ << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
	i2c_master_stop(cmd);
	esp_err_t err = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(100));
	i2c_cmd_link_delete(cmd);
	return err;
}

esp_err_t bme280_sensor::read(uint8_t reg, uint8_t *data, size_t len)
{
	if(data == NULL || len == 0) return ESP_ERR_INVALID_ARG;
	
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address_ << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg, true);
	
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address_ << 1) | I2C_MASTER_READ, true);
	if(len > 1)
	{
		i2c_master_read(cmd, data, len-1, I2C_MASTER_ACK);
	}
	
	i2c_master_read_byte(cmd, &data[len-1], I2C_MASTER_NACK);
	
	i2c_master_stop(cmd);

	esp_err_t err = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(100));
	i2c_cmd_link_delete(cmd);

	if(err != ESP_OK)
	{
			ESP_LOGE(TAG, "I2C-läsning misslyckades: %s", esp_err_to_name(err));
	}
	return err;
}

esp_err_t bme280_sensor::read_calibration()
{
	uint8_t buf[26];
	
	esp_err_t err = this->read(0x88, buf, 26);
	if(err != ESP_OK) return err;
	
	calib_.T1 = (uint16_t)(buf[1] << 8) | buf[0];
	calib_.T2 = (int16_t)(buf[3] << 8) | buf[2];
	calib_.T3 = (int16_t)(buf[5] << 8) | buf[4];
	calib_.P1 = (uint16_t)(buf[7] << 8) | buf[6];
	calib_.P2 = (int16_t)(buf[9] << 8) | buf[8];
	calib_.P3 = (int16_t)(buf[11] << 8) | buf[10];
	calib_.P4 = (int16_t)(buf[13] << 8) | buf[12];
	calib_.P5 = (int16_t)(buf[15] << 8) | buf[14];
	calib_.P6 = (int16_t)(buf[17] << 8) | buf[16];
	calib_.P7 = (int16_t)(buf[19] << 8) | buf[18];
	calib_.P8 = (int16_t)(buf[21] << 8) | buf[20];
	calib_.P9 = (int16_t)(buf[23] << 8) | buf[22];
	calib_.H1 = buf[25];
	
	err = this->read(0xE1, buf, 7);
	if(err != ESP_OK) return err;
	
	calib_.H2 = (int16_t)(buf[1] << 8) | buf[0];
	calib_.H3 = buf[2];
	calib_.H4 = ((int16_t)buf[3] << 4) | (buf[4] & 0x0F);
	calib_.H5 = ((int16_t)buf[5] << 4) | (buf[4] >> 4);
	calib_.H6 = (int8_t)buf[6];
	
	ESP_LOGI(TAG, "Kalibrering laddad: T1=%u T2=%d T3=%d", calib_.T1, calib_.T2, calib_.T3);
	
	return ESP_OK;
}

esp_err_t bme280_sensor::start_measurement()
{
	esp_err_t err = this->write(0xF2, 0x01);
	if(err != ESP_OK) return err;
	err = this->write(0xF4, 0x27);
	if(err != ESP_OK) return err;
	vTaskDelay(pdMS_TO_TICKS(10));
	return ESP_OK;
}

esp_err_t bme280_sensor::read_raw_meas(raw_measurement_t *raw)
{
	uint8_t buf[8];
	esp_err_t err = this->read(0xF7, buf, 8);
	if(err != ESP_OK) return err;
	
	raw->adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
	raw->adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);
	raw->adc_H = ((int32_t)buf[6] << 8) | buf[7];
	
	return ESP_OK;
}

int32_t bme280_sensor::compensate_T(int32_t adc_T)
{
	int32_t var1, var2, T;
	var1 = ((((adc_T>>3) - ((int32_t)calib_.T1<<1))) * ((int32_t)calib_.T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)calib_.T1)) * ((adc_T>>4) - ((int32_t)calib_.T1)))>>12)*((int32_t)calib_.T3)) >> 14;
	t_fine_ = var1 + var2;
	T = (t_fine_*5 +128) >> 8;
	return T;
}

uint32_t bme280_sensor::compensate_P(int32_t adc_P)
{
	int64_t var1, var2, P;
	var1 = ((int64_t)t_fine_) - 128000;
	var2 = var1*var1*(int64_t)calib_.P6;
	var2 = var2 + ((var1*(int64_t)calib_.P5)<<17);
	var2 = var2 + (((int64_t)calib_.P4)<<35);
	var1 = ((var1*var1*(int64_t)calib_.P3)>>8) + ((var1*(int64_t)calib_.P2) << 12);
	var1 = (((((int64_t)1)<<47)+var1))*((int64_t)calib_.P1)>>33;
	if(var1 == 0) return 0;
	P = 1048576-adc_P;
	P = (((P<<31)-var2)*3125)/var1;
	var1 = (((int64_t)calib_.P9)*(P>>13)*(P>>13)) >> 25;
	var2 = (((int64_t)calib_.P8)*P) >> 19;
	P = ((P + var1 + var2) >> 8) + (((int64_t)calib_.P7) << 4);
	return (uint32_t) P;
}

uint32_t bme280_sensor::compensate_H(int32_t adc_H)
{
	int32_t res;
	
	res = (t_fine_ - ((int32_t)76800));
	res = (((((adc_H << 14) - (((int32_t)calib_.H4) << 20) - (((int32_t)calib_.H5) * res)) + ((int32_t)16384)) >> 15) * (((((((res*((int32_t)calib_.H6)) >> 10) * (((res*((int32_t)calib_.H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)calib_.H2) + 8192) >> 14));
	res = (res - (((((res >> 15) * (res >> 15)) >> 7) * ((int32_t)calib_.H1)) >> 4));
	res = (res < 0 ? 0 : res);
	res = (res > 419430400 ? 419430400 : res);
	return (uint32_t) (res>>12);
}

esp_err_t bme280_sensor::read_measurement(sensorReading &reading)
{
	raw_measurement_t raw;
	
	esp_err_t err = this->read_raw_meas(&raw);
	if(err != ESP_OK)
	{
		ESP_LOGE(TAG, "Fick inte mätningar");
		return err;
	}
	
	reading.T = this->compensate_T(raw.adc_T);
	reading.P = this->compensate_P(raw.adc_P);
	reading.H = this->compensate_H(raw.adc_H);
	
	return ESP_OK;
}