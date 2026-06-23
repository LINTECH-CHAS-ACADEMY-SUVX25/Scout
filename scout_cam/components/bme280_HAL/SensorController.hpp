#pragma once
#include "esp_err.h"
#include "esp_log.h"

template <typename TSensor> class SensorController
{
public:
	SensorController(TSensor &sensor) : sensor_(sensor) {}
	void initSensor()
	{
		esp_err_t err = sensor_.init();
		ESP_ERROR_CHECK(err);
		ESP_LOGI(TAG_, "BME280 i2c-driver initierad");
		
		err = sensor_.read_calibration();
		if(err != ESP_OK)
		{
			ESP_LOGE(TAG_, "Kalibrering misslyckades");
			return;
		}
		
		err = sensor_.start_measurement();
		if(err != ESP_OK)
		{
			ESP_LOGE(TAG_, "Kunde inte starta mätningar");
			return;
		}
	}

	void readSensor()
	{
		sensorReading reading;
		esp_err_t err = sensor_.read_measurement(reading);
		if(err != ESP_OK)
		{
			ESP_LOGE(TAG_, "Fick inte mätningar");
		}
		else
		{
			ESP_LOGI(TAG_, "T: %u.%u DegC, P: %u Pa, H: %u %RH", reading.T/100, reading.T%100, reading.P/256, reading.H/1024);
			tooHot_ = (reading.T/100 >= 25) ? true : false;
			if(tooHot_) ESP_LOGW(TAG_, "För varmt! Öppna fönstret!");
		}
	}
	bool getTooHot() { return tooHot_; }
private:
	TSensor &sensor_;
	bool tooHot_ = false;
	const char *TAG_ = "SensorController";

};