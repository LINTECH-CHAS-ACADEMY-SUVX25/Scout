/**
 * @file sensor_sim.c
 * @brief Simulated sensor backend for Scout
 *
 * Generates realistic data with noise, drift, and periodic "events"
 * (gas leak, obstacle, heat source) for UI testing without hardware.
 */

#include "sensor_hal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ---- helpers ---- */
static float randf(float lo, float hi)
{
    return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* ---- state ---- */
static uint32_t sim_time_ms    = 0;
static float    sim_phase      = 0.0f;
static bool     evt_gas_leak   = false;
static bool     evt_obstacle   = false;
static bool     evt_heat       = false;

static float base_temp  = 22.0f;
static float base_hum   = 45.0f;
static float base_press = 1013.25f;
static float base_co    = 1.0f;
static float base_co2   = 420.0f;
static float base_pm25  = 12.0f;
static float base_dist  = 150.0f;
static float base_lux   = 5.0f;
static float battery    = 100.0f;

/* ------------------------------------------------------------------ */
int scout_sensors_init(void)
{
    srand(42);
    sim_time_ms = 0;
    sim_phase   = 0.0f;
    battery     = 100.0f;
    evt_gas_leak = evt_obstacle = evt_heat = false;
    return 0;
}

/* ------------------------------------------------------------------ */
void scout_sensors_tick(uint32_t dt_ms)
{
    sim_time_ms += dt_ms;
    sim_phase    = (float)sim_time_ms / 1000.0f;
    battery      = clampf(battery - 0.0005f * dt_ms, 0.0f, 100.0f);

    if (rand() % 600 == 0) evt_gas_leak = !evt_gas_leak;
    if (rand() % 400 == 0) evt_obstacle = !evt_obstacle;
    if (rand() % 500 == 0) evt_heat     = !evt_heat;
}

/* ------------------------------------------------------------------ */
void scout_sensors_read(scout_sensors_t *out)
{
    memset(out, 0, sizeof(*out));
    out->timestamp_ms = sim_time_ms;
    out->battery_pct  = battery;

    float drift = sinf(sim_phase * 0.1f);

    /* BME280 */
    out->env.temperature_c = base_temp  + drift * 2.0f + randf(-0.3f, 0.3f);
    out->env.humidity_pct  = clampf(base_hum + drift * 5.0f + randf(-1, 1), 0, 100);
    out->env.pressure_hpa  = base_press + drift * 1.5f + randf(-0.2f, 0.2f);
    if (evt_heat) out->env.temperature_c += 15.0f + randf(0, 5);

    /* Gas */
    out->gas.co_ppm   = clampf(base_co  + randf(-0.3f, 0.5f), 0, 500);
    out->gas.co2_ppm  = clampf(base_co2 + drift * 30.0f + randf(-10, 10), 300, 5000);
    out->gas.nh3_ppm  = clampf(randf(0, 2), 0, 100);
    out->gas.tvoc_ppb = clampf(randf(20, 80) + drift * 20.0f, 0, 2000);
    if (evt_gas_leak) {
        out->gas.co_ppm  += 40.0f + randf(0, 20);
        out->gas.co2_ppm += 800.0f;
        out->gas.tvoc_ppb += 500.0f;
    }

    /* Particles */
    out->particle.pm25 = clampf(base_pm25 + randf(-3, 3) + drift * 4.0f, 0, 500);
    out->particle.pm10 = out->particle.pm25 * randf(1.2f, 1.8f);
    if (evt_gas_leak) {
        out->particle.pm25 += 60.0f;
        out->particle.pm10 += 90.0f;
    }

    /* IMU */
    out->imu.roll_deg  = sinf(sim_phase * 0.3f) * 5.0f  + randf(-1, 1);
    out->imu.pitch_deg = cosf(sim_phase * 0.2f) * 3.0f  + randf(-1, 1);
    out->imu.yaw_deg   = fmodf(sim_phase * 8.0f, 360.0f);
    out->imu.accel_g   = 1.0f + randf(-0.05f, 0.05f);

    /* Distance */
    if (evt_obstacle) {
        out->distance.distance_cm    = randf(5, 25);
        out->distance.object_detected = true;
    } else {
        out->distance.distance_cm    = clampf(base_dist + randf(-10, 10), 2, 400);
        out->distance.object_detected = (out->distance.distance_cm < 30.0f);
    }

    /* Thermal 8×8 */
    float ambient = out->env.temperature_c;
    float tmin = 999, tmax = -999;
    for (int r = 0; r < THERMAL_ROWS; r++) {
        for (int c = 0; c < THERMAL_COLS; c++) {
            float v = ambient + randf(-1.5f, 1.5f);
            if (evt_heat && r >= 2 && r <= 5 && c >= 3 && c <= 6)
                v += 25.0f + randf(0, 8);
            out->thermal.pixels[r][c] = v;
            if (v < tmin) tmin = v;
            if (v > tmax) tmax = v;
        }
    }
    out->thermal.min_temp = tmin;
    out->thermal.max_temp = tmax;

    /* Light */
    out->light.lux = clampf(base_lux + randf(-2, 2) + drift * 3.0f, 0, 65535);
}

/* ------------------------------------------------------------------ */
alert_level_t scout_evaluate_alerts(const scout_sensors_t *d)
{
    if (d->gas.co_ppm        > 35.0f)  return ALERT_DANGER;
    if (d->gas.co2_ppm       > 2000.0f) return ALERT_DANGER;
    if (d->particle.pm25     > 150.0f) return ALERT_DANGER;
    if (d->env.temperature_c > 50.0f)  return ALERT_DANGER;
    if (d->thermal.max_temp  > 60.0f)  return ALERT_DANGER;

    if (d->gas.co_ppm        > 9.0f)   return ALERT_WARN;
    if (d->gas.co2_ppm       > 1000.0f) return ALERT_WARN;
    if (d->particle.pm25     > 55.0f)  return ALERT_WARN;
    if (d->env.temperature_c > 35.0f)  return ALERT_WARN;
    if (d->env.humidity_pct  > 80.0f)  return ALERT_WARN;
    if (d->distance.distance_cm < 30.0f) return ALERT_WARN;
    if (d->battery_pct       < 20.0f)  return ALERT_WARN;

    return ALERT_NONE;
}

/* ------------------------------------------------------------------ */
const char *scout_alert_str(alert_level_t level)
{
    switch (level) {
        case ALERT_DANGER: return "FARA";
        case ALERT_WARN:   return "VARNING";
        default:           return "OK";
    }
}
