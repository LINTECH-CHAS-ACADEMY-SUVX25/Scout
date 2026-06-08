#include "CameraEsp32.hpp"
#include "MotorEsp32.hpp"
#include "motor_task.hpp"
#include "udp_stream.hpp"

extern "C" {
#include "wifi_sta.h"
#include "monitor.h"
#include "esp_log.h"
#include "esp_err.h"
}

// main.cpp is the only place that knows the concrete hardware types.
// All tasks below receive interface pointers and never include hardware headers.
static CameraEsp32 g_camera;
static MotorEsp32  g_motor;

static const char *TAG = "scout_cam";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Scout CAM starting");

    g_motor.init();
    wifi_connect();
    ESP_ERROR_CHECK(g_camera.init() ? ESP_OK : ESP_FAIL);

    monitor_init();
    motor_task_start(&g_motor);
    udp_stream_start(&g_camera);
}
