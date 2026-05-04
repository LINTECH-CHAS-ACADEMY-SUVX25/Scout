#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lvgl.h"
#include "gt911.h"
#include "rgb_lcd_port.h"

static const char *TAG = "screen";

#define CAM_W    160
#define CAM_H    120
#define VID_PORT 3334

static esp_lcd_panel_handle_t g_panel;
static void *g_fb[2];
static esp_lcd_touch_handle_t g_touch;

/* ---- LVGL callbacks ---- */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    esp_lcd_panel_draw_bitmap(g_panel, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(drv);
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t x, y;
    uint8_t cnt = 0;
    esp_lcd_touch_read_data(g_touch);
    bool touched = esp_lcd_touch_get_coordinates(g_touch, &x, &y, NULL, &cnt, 1);
    data->point.x = x;
    data->point.y = y;
    data->state = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void lvgl_tick_task(void *arg)
{
    while (1) {
        lv_tick_inc(10);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ---- Camera feed ---- */
static lv_color_t g_canvas_buf[CAM_W * CAM_H];
static lv_obj_t  *g_canvas;
static uint8_t    g_frame[CAM_W * CAM_H * 2];  /* raw RGB565 from cam */
static volatile bool g_new_frame = false;

static void lvgl_handler_task(void *arg)
{
    while (1) {
        lv_timer_handler();
        if (g_new_frame) {
            g_new_frame = false;
            memcpy(g_canvas_buf, g_frame, sizeof(g_canvas_buf));
            lv_obj_invalidate(g_canvas);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ---- WiFi AP ---- */
static void wifi_ap_start(void)
{
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

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid           = "Scout_AP",
            .password       = "scout1234",
            .max_connection = 1,
            .authmode       = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "AP started: Scout_AP");
}

/* ---- TCP server ---- */
static esp_err_t recv_all(int sock, void *buf, size_t len)
{
    uint8_t *p = buf;
    while (len > 0) {
        int n = recv(sock, p, len, 0);
        if (n <= 0) return ESP_FAIL;
        p += n;
        len -= n;
    }
    return ESP_OK;
}

static void tcp_server_task(void *arg)
{
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(VID_PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 1);
    ESP_LOGI(TAG, "TCP server ready on port %d", VID_PORT);

    while (1) {
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;
        ESP_LOGI(TAG, "Camera connected");

        while (1) {
            uint32_t len_net;
            if (recv_all(client, &len_net, 4) != ESP_OK) break;
            uint32_t len = ntohl(len_net);
            if (len > sizeof(g_frame)) break;
            if (recv_all(client, g_frame, len) != ESP_OK) break;
            g_new_frame = true;
        }

        close(client);
        ESP_LOGW(TAG, "Camera disconnected");
    }
}

/* ---- Main ---- */
void app_main(void)
{
    wifi_ap_start();

    g_touch = touch_gt911_init();
    g_panel = waveshare_esp32_s3_rgb_lcd_init();
    waveshare_get_frame_buffer(&g_fb[0], &g_fb[1]);
    wavesahre_rgb_lcd_bl_on();

    lv_init();

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, g_fb[0], g_fb[1],
                          EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res     = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res     = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb    = lvgl_flush_cb;
    disp_drv.draw_buf    = &draw_buf;
    disp_drv.full_refresh = 1;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    /* dark blue background */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x4F48A1), 0);

    /* camera canvas centered, scaled 4x */
    g_canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(g_canvas, g_canvas_buf, CAM_W, CAM_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(g_canvas, LV_ALIGN_CENTER, 0, 0);
    lv_canvas_fill_bg(g_canvas, lv_color_hex(0x111111), LV_OPA_COVER);
    lv_img_set_pivot(g_canvas, CAM_W / 2, CAM_H / 2);
    lv_img_set_zoom(g_canvas, 640);  /* 160 = 1x, 640 = 4x */

    xTaskCreate(lvgl_tick_task,    "lvgl_tick",    2048, NULL, 5, NULL);
    xTaskCreate(lvgl_handler_task, "lvgl_handler", 4096, NULL, 4, NULL);
    xTaskCreate(tcp_server_task,   "tcp_server",   4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Screen ready");
    vTaskDelete(NULL);
}
