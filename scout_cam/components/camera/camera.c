#include "camera.h"
#include "rc_protocol.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"

// Driver wrapper for the AI-Thinker ESP32-CAM (OV2640).
// Hides esp_camera_fb_t from all task code — callers only see
// a raw byte pointer via camera_capture / camera_release.

// Centered crop of the VGA frame to CAM_W x CAM_H via OV2640 DSP windowing.
// VGA (640x480) is produced in SVGA mode from an 800x600 sensor window (offset 0,0)
// scaled 0.8x to 640x480. To crop the centered 480x480 at the same 0.8x scale:
//   horizontal: 480 output ← 600-wide window, centered in 800 → offset_x=(800-600)/2=100
//   vertical:   unchanged → 600-high window, offset_y=0, 480 output
// All window/output dims must be divisible by 4.
#define CROP_MODE_SVGA  1     // ov2640_sensor_mode_t: UXGA=0, SVGA=1, CIF=2
#define CROP_OFFSET_X   100
#define CROP_OFFSET_Y   0
#define CROP_WIN_X      600
#define CROP_WIN_Y      600

static const char *TAG = "camera";

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

static camera_fb_t *s_fb;

// Applies the centered CAM_W x CAM_H crop. Logs and leaves the sensor at VGA on failure —
// a windowing error must not stop the camera from working.
static void camera_apply_crop(void)
{
    sensor_t *s = esp_camera_sensor_get();
    if(!s || !s->set_res_raw) {
        ESP_LOGW(TAG, "sensor windowing unavailable — staying at VGA");
        return;
    }
    int ret = s->set_res_raw(s, CROP_MODE_SVGA, 0, 0, 0,
                             CROP_OFFSET_X, CROP_OFFSET_Y,
                             CROP_WIN_X, CROP_WIN_Y,
                             CAM_W, CAM_H, false, false);
    if(ret != 0) ESP_LOGW(TAG, "set_res_raw failed (%d) — staying at VGA", ret);
}

void camera_init(void)
{
    camera_config_t config = {
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
        .xclk_freq_hz = 24000000,
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size   = FRAMESIZE_VGA,  // SVGA=800x600, VGA=640x480, CIF=352x288, QVGA=320x240, HQVGA=240x176
        .jpeg_quality = 20,               // lower = smaller payload, higher = better quality
        .fb_count     = 2,                // capture into one buffer while sending the other
        .fb_location  = CAMERA_FB_IN_PSRAM,
        .grab_mode    = CAMERA_GRAB_LATEST, // always get the newest frame, skip older ones
    };
    esp_err_t err = esp_camera_init(&config);
    if(err != ESP_OK) {
    ESP_LOGE(TAG, "camera init failed (%s) — rebooting", esp_err_to_name(err));
    esp_restart();
    }
    camera_apply_crop();
    ESP_LOGI(TAG, "camera ready (%dx%d crop)", CAM_W, CAM_H);
}

bool camera_capture(const uint8_t **buf, size_t *len)
{
    s_fb = esp_camera_fb_get();
    if(!s_fb) return false;
    *buf = s_fb->buf;
    *len = s_fb->len;
    return true;
}

void camera_release(void)
{
    if(s_fb) {
        esp_camera_fb_return(s_fb);
        s_fb = NULL;
    }
}
