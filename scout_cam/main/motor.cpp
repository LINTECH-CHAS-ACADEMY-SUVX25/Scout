#include "motor.h"
#include "motor_queue.h"
#include "motor_driver.hpp"
#include "MotorController.hpp"
#include "watchdog.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — receives joystick x/y packets from the command queue and drives the
// L298N H-bridge through the motor HAL. Times out after 500 ms and stops as a
// safety measure so the car doesn't run away if the dashboard disconnects.

static const char *TAG = "motor";

static MotorDriver driver;
static MotorController<MotorDriver> controller(driver);

static void motor_run(void *arg)
{
    watchdog_register();
    controller.stop();

    while(1) {
        int16_t x, y;
        if(motor_queue_recv(&x, &y, 500)) {
            controller.drive(x, y);
        } else if(controller.isMoving()) {
            controller.stop();
            ESP_LOGW(TAG, "command timeout — motors stopped");
        }
        watchdog_reset();
    }
}

extern "C" void motor_init(void)
{
    controller.initMotor();
    motor_queue_init();
    xTaskCreate(motor_run, "motor", 2048, NULL, 6, NULL);
    ESP_LOGI(TAG, "motor ready");
}
