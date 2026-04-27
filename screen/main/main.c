#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "i2c.h"
#include "io_extension.h"
#include "rgb_lcd_port.h"
#include "font8x8.h"

static const char *MAIN_TAG = "scout_ap";

/* ---- WiFi AP config ---- */
#define AP_SSID     "Scout_AP"
#define AP_PASS     "scout1234"
#define AP_CHAN     1
#define AP_MAX_STA  4
#define UDP_PORT    3333

/* ---- Display ---- */
#define LCD_W       EXAMPLE_LCD_H_RES
#define LCD_H       EXAMPLE_LCD_V_RES
#define FONT_SCALE  2
#define CHAR_W      (8 * FONT_SCALE)
#define CHAR_H      (8 * FONT_SCALE)
#define LINE_H      (CHAR_H + 4)

#define COL_BG      0x0000U
#define COL_WHITE   0xFFFFU
#define COL_YELLOW  0xFFE0U
#define COL_CYAN    0x07FFU
#define COL_GREEN   0x07E0U
#define COL_GRAY    0x8410U

static uint16_t *g_fb;

static void fb_draw_char(int x0, int y0, const uint8_t *glyph, uint16_t color)
{
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (!(bits & (0x01u << col))) continue;
            for (int dy = 0; dy < FONT_SCALE; dy++) {
                int py = y0 + row * FONT_SCALE + dy;
                if ((unsigned)py >= LCD_H) continue;
                for (int dx = 0; dx < FONT_SCALE; dx++) {
                    int px = x0 + col * FONT_SCALE + dx;
                    if ((unsigned)px >= LCD_W) continue;
                    g_fb[py * LCD_W + px] = color;
                }
            }
        }
    }
}

static void fb_draw_str(int x, int y, const char *s, uint16_t color)
{
    while (*s) {
        fb_draw_char(x, y, font8x8_glyph(*s), color);
        x += CHAR_W;
        s++;
    }
}

static void fb_hline(int y, uint16_t color)
{
    if ((unsigned)y >= LCD_H) return;
    for (int x = 0; x < LCD_W; x++) g_fb[y * LCD_W + x] = color;
}

/* ---- Shared state written by UDP task ---- */
static char     g_last_msg[80]   = "(none)";
static char     g_last_ip[20]    = "";

/* ---- Display render ---- */
static void render(int client_count)
{
    memset(g_fb, 0, LCD_W * LCD_H * sizeof(uint16_t));

    /* Header */
    fb_draw_str(8, 8, "SCOUT  Access Point", COL_CYAN);
    fb_hline(8 + CHAR_H + 4, COL_CYAN);

    /* AP info */
    int y = 8 + CHAR_H + 12;
    fb_draw_str(8,             y, "SSID:    ", COL_YELLOW);
    fb_draw_str(8 + 9*CHAR_W,  y, AP_SSID,    COL_WHITE);
    y += LINE_H;
    fb_draw_str(8,             y, "Password:", COL_YELLOW);
    fb_draw_str(8 + 9*CHAR_W,  y, AP_PASS,    COL_WHITE);
    y += LINE_H;
    fb_draw_str(8,             y, "AP IP:   ", COL_YELLOW);
    fb_draw_str(8 + 9*CHAR_W,  y, "192.168.4.1", COL_WHITE);

    y += LINE_H + 4;
    fb_hline(y, COL_GRAY);
    y += 8;

    /* Connected clients */
    char buf[48];
    snprintf(buf, sizeof(buf), "Clients connected: %d / %d", client_count, AP_MAX_STA);
    fb_draw_str(8, y, buf, client_count > 0 ? COL_GREEN : COL_YELLOW);

    y += LINE_H + 4;
    fb_hline(y, COL_GRAY);
    y += 8;

    /* Last message */
    fb_draw_str(8, y, "Last message:", COL_YELLOW);
    y += LINE_H;
    fb_draw_str(8, y, g_last_msg, COL_WHITE);
    if (g_last_ip[0]) {
        y += LINE_H;
        snprintf(buf, sizeof(buf), "from %s", g_last_ip);
        fb_draw_str(8, y, buf, COL_GRAY);
    }
}

/* ---- UDP server task ---- */
static void udp_server_task(void *arg)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(MAIN_TAG, "socket() failed");
        vTaskDelete(NULL);
    }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(UDP_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    ESP_LOGI(MAIN_TAG, "UDP server listening on port %d", UDP_PORT);

    char buf[64];
    struct sockaddr_in src;
    socklen_t src_len = sizeof(src);

    while (1) {
        int len = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                           (struct sockaddr *)&src, &src_len);
        if (len > 0) {
            buf[len] = '\0';
            strncpy(g_last_msg, buf, sizeof(g_last_msg) - 1);
            strncpy(g_last_ip, inet_ntoa(src.sin_addr), sizeof(g_last_ip) - 1);
            ESP_LOGI(MAIN_TAG, "RX [%s]: %s", g_last_ip, g_last_msg);
        }
    }
}

/* ---- WiFi AP init ---- */
static void wifi_init_ap(void)
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

    wifi_config_t ap_config = {
        .ap = {
            .ssid           = AP_SSID,
            .ssid_len       = sizeof(AP_SSID) - 1,
            .channel        = AP_CHAN,
            .password       = AP_PASS,
            .max_connection = AP_MAX_STA,
            .authmode       = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(MAIN_TAG, "AP started: SSID=%s  pass=%s", AP_SSID, AP_PASS);
}

/* ---- Main ---- */
void app_main(void)
{
    DEV_I2C_Init();
    IO_EXTENSION_Init();
    waveshare_esp32_s3_rgb_lcd_init();

    void *buf1 = NULL, *buf2 = NULL;
    waveshare_get_frame_buffer(&buf1, &buf2);
    g_fb = (uint16_t *)buf1;

    memset(g_fb, 0, LCD_W * LCD_H * sizeof(uint16_t));
    fb_draw_str(8, 8, "Starting AP...", COL_CYAN);
    wavesahre_rgb_lcd_bl_on();

    wifi_init_ap();

    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);

    wifi_sta_list_t sta_list;
    while (1) {
        esp_wifi_ap_get_sta_list(&sta_list);
        render(sta_list.num);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
