#include "wifi.h"
#include "rc_protocol.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static const char *TAG = "wifi";

// Starta WiFi som Access Point — screenen äger nätverket, kameran ansluter till oss.
// Det ger oss en fast IP (192.168.4.1) som kameran alltid skickar video till.
void wifi_ap_start(void)
{
    // NVS behöver initieras innan WiFi-drivrutinen startar
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // AP-konfiguration: SSID och lösenord hämtas från rc_protocol.h
    // max_connection=1 eftersom vi bara vill ha kameran inkopplad
    wifi_config_t ap_cfg = 
    {
        .ap = 
        {
            .ssid           = AP_SSID,
            .password       = AP_PASS,
            .max_connection = 1,
            .authmode       = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    // Stäng av WiFi power save — annars kan ESP32 sova i 50–100 ms (DTIM-intervall)
    // mellan mottagna paket, vilket ger kraftig latens i videoströmmen
    esp_wifi_set_ps(WIFI_PS_NONE);
    ESP_LOGI(TAG, "AP started: %s", AP_SSID);
}
