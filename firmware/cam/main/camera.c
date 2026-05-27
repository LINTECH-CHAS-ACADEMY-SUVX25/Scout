#include "camera.h"
#include "esp_camera.h"

// AI-Thinker ESP32-CAM
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// Initierar OV2640-kameran på AI-Thinker ESP32-CAM.
// Inställningarna är optimerade för högsta möjliga FPS med acceptabel bildkvalitet.
esp_err_t camera_init(void)
{
    camera_config_t config = 
    {
        .pin_pwdn     = CAM_PIN_PWDN,
        .pin_reset    = CAM_PIN_RESET,
        .pin_xclk     = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7, .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5, .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3, .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1, .pin_d0 = CAM_PIN_D0,
        .pin_vsync    = CAM_PIN_VSYNC,
        .pin_href     = CAM_PIN_HREF,
        .pin_pclk     = CAM_PIN_PCLK,
        .xclk_freq_hz = 20000000,
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        // HQVGA (240x176) — originalinställning, mindre JPEG = högre FPS.
        .frame_size   = FRAMESIZE_HQVGA,
        // Kvalitet 20 — originalinställning.
        // Skala 0–63 där lägre tal ger bättre kvalitet men större paket.
        .jpeg_quality = 20,
        // Två bildbuffertar: kameran fyller en medan vi skickar den andra
        .fb_count     = 2,
        .fb_location  = CAMERA_FB_IN_PSRAM,
        // Alltid ta den senaste bilden — hoppa över gamla bilder i kön
        // för att hålla latensen nere
        .grab_mode    = CAMERA_GRAB_LATEST,
    };
    return esp_camera_init(&config);
}
