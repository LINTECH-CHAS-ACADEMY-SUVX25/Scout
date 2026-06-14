#include "bme280.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdint.h>

// Bosch BME280 driver over the new i2c_master API (SDA=GPIO2, SCL=GPIO3, addr 0x77).
// A missing or unresponsive sensor is never fatal: bme280_read() returns false
// and the cam keeps streaming with diag fields at zero.

#define BME280_ADDR  0x77
#define PIN_SDA      GPIO_NUM_2
#define PIN_SCL      GPIO_NUM_3
#define I2C_FREQ     100000
#define I2C_TIMEOUT  100          // ms per transaction

#define REG_ID       0xD0         // expect 0x60
#define REG_RESET    0xE0
#define REG_CTRL_HUM 0xF2
#define REG_STATUS   0xF3
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG   0xF5
#define REG_DATA     0xF7         // 8-byte burst: press / temp / hum
#define REG_CALIB_TP 0x88         // dig_T1..dig_P9 (24 bytes) + dig_H1 at 0xA1
#define REG_CALIB_H1 0xA1
#define REG_CALIB_H  0xE1         // dig_H2..dig_H6 (7 bytes)

#define CTRL_HUM_X1  0x01                 // humidity oversampling x1
#define CTRL_MEAS_FORCED 0x25             // temp x1 | press x1 | forced mode
#define STATUS_MEASURING 0x08

static const char *TAG = "bme280";

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;
static bool s_ready;
static int32_t s_t_fine;

static struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t  H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} s_cal;

static esp_err_t reg_read(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, len, I2C_TIMEOUT);
}

static esp_err_t reg_write(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = { reg, val };
    return i2c_master_transmit(s_dev, data, sizeof(data), I2C_TIMEOUT);
}

static esp_err_t read_calib(void)
{
    uint8_t tp[24], h1, h[7];
    esp_err_t err = reg_read(REG_CALIB_TP, tp, sizeof(tp));
    if(err != ESP_OK) return err;
    err = reg_read(REG_CALIB_H1, &h1, 1);
    if(err != ESP_OK) return err;
    err = reg_read(REG_CALIB_H, h, sizeof(h));
    if(err != ESP_OK) return err;

    s_cal.T1 = (uint16_t)(tp[1] << 8 | tp[0]);
    s_cal.T2 = (int16_t)(tp[3] << 8 | tp[2]);
    s_cal.T3 = (int16_t)(tp[5] << 8 | tp[4]);
    s_cal.P1 = (uint16_t)(tp[7] << 8 | tp[6]);
    s_cal.P2 = (int16_t)(tp[9] << 8 | tp[8]);
    s_cal.P3 = (int16_t)(tp[11] << 8 | tp[10]);
    s_cal.P4 = (int16_t)(tp[13] << 8 | tp[12]);
    s_cal.P5 = (int16_t)(tp[15] << 8 | tp[14]);
    s_cal.P6 = (int16_t)(tp[17] << 8 | tp[16]);
    s_cal.P7 = (int16_t)(tp[19] << 8 | tp[18]);
    s_cal.P8 = (int16_t)(tp[21] << 8 | tp[20]);
    s_cal.P9 = (int16_t)(tp[23] << 8 | tp[22]);
    s_cal.H1 = h1;
    s_cal.H2 = (int16_t)(h[1] << 8 | h[0]);
    s_cal.H3 = h[2];
    s_cal.H4 = (int16_t)((int8_t)h[3] << 4 | (h[4] & 0x0F));
    s_cal.H5 = (int16_t)((int8_t)h[5] << 4 | (h[4] >> 4));
    s_cal.H6 = (int8_t)h[6];
    return ESP_OK;
}

// Float compensation, transcribed from the BME280 datasheet (section 4.2.3).
static float compensate_temp(int32_t adc_T)
{
    float v1 = ((float)adc_T / 16384.0f - (float)s_cal.T1 / 1024.0f) * (float)s_cal.T2;
    float v2 = ((float)adc_T / 131072.0f - (float)s_cal.T1 / 8192.0f);
    v2 = v2 * v2 * (float)s_cal.T3;
    s_t_fine = (int32_t)(v1 + v2);
    return (v1 + v2) / 5120.0f;
}

static float compensate_press(int32_t adc_P)
{
    float v1 = ((float)s_t_fine / 2.0f) - 64000.0f;
    float v2 = v1 * v1 * (float)s_cal.P6 / 32768.0f;
    v2 = v2 + v1 * (float)s_cal.P5 * 2.0f;
    v2 = (v2 / 4.0f) + ((float)s_cal.P4 * 65536.0f);
    v1 = ((float)s_cal.P3 * v1 * v1 / 524288.0f + (float)s_cal.P2 * v1) / 524288.0f;
    v1 = (1.0f + v1 / 32768.0f) * (float)s_cal.P1;
    if(v1 == 0.0f) return 0.0f;   // avoid divide by zero
    float p = 1048576.0f - (float)adc_P;
    p = (p - (v2 / 4096.0f)) * 6250.0f / v1;
    v1 = (float)s_cal.P9 * p * p / 2147483648.0f;
    v2 = p * (float)s_cal.P8 / 32768.0f;
    return p + (v1 + v2 + (float)s_cal.P7) / 16.0f;
}

static float compensate_hum(int32_t adc_H)
{
    float h = (float)s_t_fine - 76800.0f;
    h = ((float)adc_H - ((float)s_cal.H4 * 64.0f + (float)s_cal.H5 / 16384.0f * h)) *
        ((float)s_cal.H2 / 65536.0f *
         (1.0f + (float)s_cal.H6 / 67108864.0f * h *
          (1.0f + (float)s_cal.H3 / 67108864.0f * h)));
    h = h * (1.0f - (float)s_cal.H1 * h / 524288.0f);
    if(h > 100.0f) h = 100.0f;
    else if(h < 0.0f) h = 0.0f;
    return h;
}

bool bme280_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = I2C_NUM_0,
        .scl_io_num                   = PIN_SCL,
        .sda_io_num                   = PIN_SDA,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true,
    };
    if(i2c_new_master_bus(&bus_cfg, &s_bus) != ESP_OK) {
        ESP_LOGW(TAG, "I2C bus init failed; no environmental data");
        return false;
    }
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BME280_ADDR,
        .scl_speed_hz    = I2C_FREQ,
    };
    if(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev) != ESP_OK) {
        ESP_LOGW(TAG, "I2C device add failed; no environmental data");
        return false;
    }

    uint8_t id = 0;
    if(reg_read(REG_ID, &id, 1) != ESP_OK || id != 0x60) {
        ESP_LOGW(TAG, "BME280 not found (id 0x%02x); scanning bus", id);
        for(uint8_t a = 0x08; a < 0x78; a++)
            if(i2c_master_probe(s_bus, a, 50) == ESP_OK)
                ESP_LOGW(TAG, "  I2C device ACK at 0x%02x", a);
        ESP_LOGW(TAG, "scan done; no environmental data");
        return false;
    }
    if(read_calib() != ESP_OK) {
        ESP_LOGW(TAG, "calibration read failed; no environmental data");
        return false;
    }
    // ctrl_hum must be written before ctrl_meas to take effect.
    if(reg_write(REG_CTRL_HUM, CTRL_HUM_X1) != ESP_OK ||
       reg_write(REG_CONFIG, 0x00) != ESP_OK) {
        ESP_LOGW(TAG, "config write failed; no environmental data");
        return false;
    }

    s_ready = true;
    ESP_LOGI(TAG, "BME280 ready on SDA=%d SCL=%d (0x%02x)", PIN_SDA, PIN_SCL, BME280_ADDR);
    return true;
}

bool bme280_read(float *temp, float *hum, float *pres)
{
    if(!s_ready) return false;

    // Trigger a single forced-mode measurement, then wait for it to complete.
    if(reg_write(REG_CTRL_MEAS, CTRL_MEAS_FORCED) != ESP_OK) return false;
    for(int i = 0; i < 10; i++) {
        uint8_t status = 0;
        if(reg_read(REG_STATUS, &status, 1) != ESP_OK) return false;
        if(!(status & STATUS_MEASURING)) break;
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    uint8_t d[8];
    if(reg_read(REG_DATA, d, sizeof(d)) != ESP_OK) return false;

    int32_t adc_P = (int32_t)d[0] << 12 | (int32_t)d[1] << 4 | (d[2] >> 4);
    int32_t adc_T = (int32_t)d[3] << 12 | (int32_t)d[4] << 4 | (d[5] >> 4);
    int32_t adc_H = (int32_t)d[6] << 8 | d[7];

    float t = compensate_temp(adc_T);   // sets s_t_fine, used by press/hum below
    if(temp) *temp = t;
    if(pres) *pres = compensate_press(adc_P);
    if(hum)  *hum  = compensate_hum(adc_H);
    return true;
}
