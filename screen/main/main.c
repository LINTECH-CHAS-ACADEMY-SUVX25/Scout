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

static const char *MAIN_TAG = "scout_screen";

#define AP_SSID      "Scout_AP"
#define AP_PASS      "scout1234"
#define AP_MAX_STA   4
#define VID_PORT     3334

#define LCD_W        EXAMPLE_LCD_H_RES
#define LCD_H        EXAMPLE_LCD_V_RES
#define FONT_SCALE   2
#define CHAR_W       (8 * FONT_SCALE)
#define CHAR_H       (8 * FONT_SCALE)

#define COL_CYAN     0x07FFU
#define COL_YELLOW   0xFFE0U
#define COL_GRAY     0x8410U

/* ---- Double-buffer state ---- */
static esp_lcd_panel_handle_t g_panel;
static void    *g_fbs[2];      /* both DMA framebuffers */
static int      g_back;        /* index of the buffer we write to */
static uint16_t *g_fb;         /* always == g_fbs[g_back] */
static SemaphoreHandle_t g_vsync_sem;

/* Vsync ISR → give semaphore so render task can swap on the right tick */
static bool IRAM_ATTR vsync_cb(esp_lcd_panel_handle_t panel,
                                const esp_lcd_rgb_panel_event_data_t *data,
                                void *user_ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)user_ctx, &woken);
    return woken == pdTRUE;
}

/*
 * Present the current back buffer: schedule the driver to flip to it on the
 * next vsync, wait for the flip to complete, then switch g_fb to the other
 * buffer.  Passing g_fbs[g_back] as the bitmap source is a zero-copy trick —
 * the driver writes to the *inactive* buffer; when source == inactive, it is a
 * self-copy (no-op) and the driver simply changes which buffer the DMA reads.
 */
static void present_frame(void)
{
    /* With a bounce-buffer RGB panel, draw_bitmap() updates the PSRAM read
     * pointer IMMEDIATELY (not deferred to the next vsync).  Calling it
     * mid-frame lets the bounce DMA switch buffers while active lines are
     * already being scanned → tear.
     *
     * Fix: wait for vsync first (we are now in the ~2.8 ms blanking window),
     * THEN swap the buffer pointer.  The bounce DMA is refilling SRAM during
     * blanking, so it picks up the new source before the first active line. */
    xSemaphoreTake(g_vsync_sem, 0);                    /* drain any stale signal */
    xSemaphoreTake(g_vsync_sem, pdMS_TO_TICKS(100));   /* wait — blanking starts  */
    esp_lcd_panel_draw_bitmap(g_panel, 0, 0, LCD_W, LCD_H, g_fbs[g_back]);
    g_back ^= 1;
    g_fb = (uint16_t *)g_fbs[g_back];
}

/* ---- Font rendering ---- */
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

/* ---- Video area geometry ---- */
#define FRAME_W     160
#define FRAME_H     120
#define VID_SCALE   4
#define VID_DST_W   (FRAME_W * VID_SCALE)          /* 640 */
#define VID_DST_H   (FRAME_H * VID_SCALE)          /* 480 */
#define VID_X       ((LCD_W - VID_DST_W) / 2)      /* 192 */
#define VID_Y       ((LCD_H - VID_DST_H) / 2)      /* 60  */
#define FRAME_BYTES (FRAME_W * FRAME_H * 2)

/*
 * Scale 160×120 → 640×480 into the current back buffer.
 * Uses a SRAM line buffer + memcpy rows for cache-friendly PSRAM writes.
 * OV2640 outputs RGB565 big-endian; we byte-swap each pixel.
 */
static void blit_frame(const uint8_t *src)
{
    static uint16_t line[VID_DST_W];   /* 1 280 B — stays in cache */

    for (int sy = 0; sy < FRAME_H; sy++) {
        /* Build one expanded row in SRAM */
        for (int sx = 0; sx < FRAME_W; sx++) {
            const uint8_t *p = src + (sy * FRAME_W + sx) * 2;
            uint16_t px = ((uint16_t)p[0] << 8) | p[1];   /* BE → LE swap */
            line[sx * VID_SCALE + 0] = px;
            line[sx * VID_SCALE + 1] = px;
            line[sx * VID_SCALE + 2] = px;
            line[sx * VID_SCALE + 3] = px;
        }
        /* Copy that row to VID_SCALE consecutive PSRAM rows */
        for (int rep = 0; rep < VID_SCALE; rep++) {
            uint16_t *dst = g_fb + (VID_Y + sy * VID_SCALE + rep) * LCD_W + VID_X;
            memcpy(dst, line, VID_DST_W * sizeof(uint16_t));
        }
    }
}

/* Draw static header/footer chrome onto whichever buffer g_fb currently is */
static void draw_chrome_to_current(void)
{
    memset(g_fb, 0, LCD_W * LCD_H * sizeof(uint16_t));
    fb_draw_str(8, (VID_Y - CHAR_H) / 2, "SCOUT  Camera Feed", COL_CYAN);

    const char *footer = "AP: Scout_AP  |  192.168.4.1";
    int fw = (int)strlen(footer) * CHAR_W;
    int fy = VID_Y + VID_DST_H + (LCD_H - VID_Y - VID_DST_H - CHAR_H) / 2;
    fb_draw_str((LCD_W - fw) / 2, fy, footer, COL_GRAY);
}

static void draw_waiting_to_current(void)
{
    /* Clear only the video area */
    for (int y = VID_Y; y < VID_Y + VID_DST_H; y++)
        memset(g_fb + y * LCD_W + VID_X, 0, VID_DST_W * sizeof(uint16_t));

    const char *msg = "Waiting for camera...";
    int mw = (int)strlen(msg) * CHAR_W;
    fb_draw_str(VID_X + (VID_DST_W - mw) / 2,
                VID_Y + (VID_DST_H - CHAR_H) / 2, msg, COL_YELLOW);
}

/* ---- TCP helpers ---- */
static esp_err_t recv_all(int sock, void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    while (len > 0) {
        int n = recv(sock, p, len, 0);
        if (n <= 0) return ESP_FAIL;
        p += n; len -= n;
    }
    return ESP_OK;
}

/* ---- Video server task ---- */
static void video_server_task(void *arg)
{
    static uint8_t frame_buf[FRAME_BYTES];

    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(VID_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 1);

    while (1) {
        /* Show waiting message in both buffers so neither shows garbage */
        draw_waiting_to_current();
        present_frame();
        draw_waiting_to_current();

        struct sockaddr_in ca;
        socklen_t clen = sizeof(ca);
        int client = accept(server, (struct sockaddr *)&ca, &clen);
        ESP_LOGI(MAIN_TAG, "Camera connected: %s", inet_ntoa(ca.sin_addr));

        while (1) {
            uint32_t len_net;
            if (recv_all(client, &len_net, 4) != ESP_OK) break;
            uint32_t flen = ntohl(len_net);
            if (flen != FRAME_BYTES) { ESP_LOGW(MAIN_TAG, "Bad frame size"); break; }
            if (recv_all(client, frame_buf, FRAME_BYTES) != ESP_OK) break;

            blit_frame(frame_buf);
            present_frame();          /* vsync-safe swap */
        }

        close(client);
        ESP_LOGW(MAIN_TAG, "Camera disconnected");
    }
}

/* ---- WiFi AP ---- */
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

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = AP_SSID, .ssid_len = sizeof(AP_SSID) - 1,
            .channel = 1, .password = AP_PASS,
            .max_connection = AP_MAX_STA, .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/* ---- Main ---- */
void app_main(void)
{
    DEV_I2C_Init();
    IO_EXTENSION_Init();
    g_panel = waveshare_esp32_s3_rgb_lcd_init();

    /* Get both DMA framebuffers */
    waveshare_get_frame_buffer(&g_fbs[0], &g_fbs[1]);
    g_back = 1;                           /* start writing to buf[1] */
    g_fb   = (uint16_t *)g_fbs[g_back];

    /* Register vsync callback */
    g_vsync_sem = xSemaphoreCreateBinary();
    esp_lcd_rgb_panel_event_callbacks_t cbs = { .on_vsync = vsync_cb };
    esp_lcd_rgb_panel_register_event_callbacks(g_panel, &cbs, g_vsync_sem);

    /* Prime both buffers with chrome so neither shows uninitialised PSRAM */
    draw_chrome_to_current();
    present_frame();
    draw_chrome_to_current();

    wavesahre_rgb_lcd_bl_on();
    wifi_init_ap();

    xTaskCreate(video_server_task, "video_srv", 8192, NULL, 5, NULL);
    vTaskDelete(NULL);
}
