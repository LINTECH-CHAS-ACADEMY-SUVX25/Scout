#include "jpeg_decode.h"
#include "esp_jpeg_dec.h"
#include "esp_log.h"

static const char *TAG = "jpeg_decode";

bool jpeg_decode_rgb565(const uint8_t *inbuf, int inbuf_len,
                        uint8_t *outbuf, size_t outbuf_size,
                        uint16_t *out_width, uint16_t *out_height)
{
    jpeg_dec_config_t cfg = DEFAULT_JPEG_DEC_CONFIG();
    cfg.output_type = JPEG_PIXEL_FORMAT_RGB565_LE;

    jpeg_dec_handle_t dec = NULL;
    jpeg_error_t ret = jpeg_dec_open(&cfg, &dec);
    if (ret != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "open failed: %d", ret);
        return false;
    }

    jpeg_dec_io_t io = {
        .inbuf     = (uint8_t *)inbuf,
        .inbuf_len = inbuf_len,
        .outbuf    = outbuf,
    };
    jpeg_dec_header_info_t info;

    ret = jpeg_dec_parse_header(dec, &io, &info);
    if (ret != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "header parse failed: %d", ret);
        goto done;
    }

    size_t needed = (size_t)info.width * info.height * 2;
    if (needed > outbuf_size) {
        ESP_LOGE(TAG, "output buffer too small: need %zu have %zu", needed, outbuf_size);
        ret = JPEG_ERR_NO_MEM;
        goto done;
    }

    ret = jpeg_dec_process(dec, &io);
    if (ret != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "decode failed: %d", ret);
        goto done;
    }

    if (out_width)  *out_width  = info.width;
    if (out_height) *out_height = info.height;

done:
    jpeg_dec_close(dec);
    return ret == JPEG_ERR_OK;
}
