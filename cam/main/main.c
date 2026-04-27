#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#define AP_SSID     "Scout_AP"
#define AP_PASS     "scout1234"
#define S3_IP       "192.168.4.1"
#define UDP_PORT    3333

static const char *TAG = "scout_cam";

static EventGroupHandle_t s_wifi_events;
#define CONNECTED_BIT  BIT0

static void on_wifi_event(void *arg, esp_event_base_t base,
                          int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Disconnected — retrying");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Connected — IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_wifi_events, CONNECTED_BIT);
    }
}

static void wifi_connect(void)
{
    s_wifi_events = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi_event, NULL, NULL));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid     = AP_SSID,
            .password = AP_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to %s...", AP_SSID);
    xEventGroupWaitBits(s_wifi_events, CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Scout CAM starting");
    wifi_connect();

    /* UDP socket aimed at the S3 AP */
    struct sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_port   = htons(UDP_PORT),
    };
    inet_pton(AF_INET, S3_IP, &dest.sin_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed");
        return;
    }
    ESP_LOGI(TAG, "Sending pings to %s:%d", S3_IP, UDP_PORT);

    int seq = 0;
    while (1) {
        char msg[32];
        snprintf(msg, sizeof(msg), "PING %d", seq++);

        int err = sendto(sock, msg, strlen(msg), 0,
                         (struct sockaddr *)&dest, sizeof(dest));
        if (err < 0) {
            ESP_LOGE(TAG, "sendto failed (errno %d)", errno);
        } else {
            ESP_LOGI(TAG, "Sent: %s", msg);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
