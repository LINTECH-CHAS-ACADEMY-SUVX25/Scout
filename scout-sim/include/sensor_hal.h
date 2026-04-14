/**
 * @file sensor_hal.h
 * @brief Scout Sensor Hardware Abstraction Layer
 *
 * Unified interface for all Scout sensors.
 * SCOUT_SIM defined  → simulated data (PC / SDL)
 * SCOUT_SIM undefined → real hardware via I²C / SPI
 */

#ifndef SENSOR_HAL_H
#define SENSOR_HAL_H

#include <stdint.h>
#include <stdbool.h>

/* ──────────── BME280 ──────────── */
typedef struct {
    float temperature_c;     /* °C   */
    float humidity_pct;      /* %RH  */
    float pressure_hpa;      /* hPa  */
} bme280_data_t;

/* ──────────── Gas (MQ-2 / MiCS-6814) ──────────── */
typedef struct {
    float co_ppm;
    float co2_ppm;
    float nh3_ppm;
    float tvoc_ppb;
} gas_data_t;

/* ──────────── Particles (PMS5003) ──────────── */
typedef struct {
    float pm25;              /* µg/m³ */
    float pm10;              /* µg/m³ */
} particle_data_t;

/* ──────────── IMU (MPU-6050 / BNO055) ──────────── */
typedef struct {
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float accel_g;
} imu_data_t;

/* ──────────── Distance (VL53L0X) ──────────── */
typedef struct {
    float distance_cm;
    bool  object_detected;
} distance_data_t;

/* ──────────── Thermal camera (AMG8833  8×8) ──────────── */
#define THERMAL_ROWS 8
#define THERMAL_COLS 8
typedef struct {
    float pixels[THERMAL_ROWS][THERMAL_COLS];
    float max_temp;
    float min_temp;
} thermal_data_t;

/* ──────────── Ambient light (BH1750) ──────────── */
typedef struct {
    float lux;
} light_data_t;

/* ──────────── Combined snapshot ──────────── */
typedef struct {
    bme280_data_t   env;
    gas_data_t      gas;
    particle_data_t particle;
    imu_data_t      imu;
    distance_data_t distance;
    thermal_data_t  thermal;
    light_data_t    light;
    uint32_t        timestamp_ms;
    float           battery_pct;
} scout_sensors_t;

/* ──────────── Alert ──────────── */
typedef enum {
    ALERT_NONE = 0,
    ALERT_WARN,
    ALERT_DANGER
} alert_level_t;

/* ──────────── Joystick input ──────────── */
typedef struct {
    float x;    /* -1.0 (left)  … +1.0 (right)  */
    float y;    /* -1.0 (back)  … +1.0 (forward) */
} joystick_input_t;

/* ──────────── API ──────────── */
int           scout_sensors_init  (void);
void          scout_sensors_read  (scout_sensors_t *out);
alert_level_t scout_evaluate_alerts(const scout_sensors_t *data);
const char   *scout_alert_str     (alert_level_t level);
void          scout_sensors_tick  (uint32_t dt_ms);

#endif /* SENSOR_HAL_H */
