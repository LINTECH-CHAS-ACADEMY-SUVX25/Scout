#include "wifi.h"
#include "motor.h"
#include "camera.h"
#include "motor_task.h"
#include "udp_stream.h"
#include "monitor.h"
#include "telemetry.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "scout_cam";

void app_main(void)
{
    ESP_LOGI(TAG, "Scout CAM starting");

    // Motor måste initieras tidigt så att vi kan skicka STOP-kommando
    // om kameran eller nätverket misslyckas med att starta
    motor_init();

    // WiFi blockerar här tills vi har en IP-adress — ingenting annat kan
    // starta förrän vi är anslutna till screenens AP
    wifi_connect();

    ESP_ERROR_CHECK(camera_init());
    ESP_LOGI(TAG, "Camera ready");

    // Starta bakgrundstaskar: systemövervakning, telemetri, motorstyrning och videoström
    monitor_start();
    telemetry_start();
    motor_task_start();
    udp_stream_start();
}
