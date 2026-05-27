#include "udp_video.h"
#include "ui.h"
#include "rc_protocol.h"
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "lwip/sockets.h"
#include "lvgl.h"
#include "jpeg_decoder.h"

// UDP-videomottagning för Scout RC-screen (ESP32-S3)
//
// Vi använder ett ping-pong-system med två bildbuffertar:
//   s_asm_buf  — hit skriver UDP-tasken inkommande fragment medan en bild byggs upp
//   s_dec_buf  — härifrån läser JPEG-avkodaren när en komplett bild kommit in
//
// När alla fragment för en bild har tagits emot byter vi bara pekarna (snabb swap
// under mutex) så att avkodaren direkt kan börja på den nya bilden medan UDP-tasken
// redan börjar fylla den gamla bufferten igen. Mutex hålls ALDRIG under
// JPEG-avkodningen (20–50 ms) — bara under swap + flagguppdatering.

// Fragmentstorlek anpassad till WiFi MTU (1500) minus IP/UDP-header (28 bytes) = 1472,
// minus lite marginal för driver-overhead. Fragment 0 bär 5 extra header-bytes (magic + längd).
#define FRAG_SIZE   1460
#define FIRST_DATA  (FRAG_SIZE - 5)
#define FRAME_BUF   (32 * 1024)
#define MAX_FRAGS   23    /* ceil((FRAME_BUF - FIRST_DATA) / FRAG_SIZE) + 1 */
#define PKT_MAX     (4 + 5 + FRAG_SIZE)

static const char *TAG = "udp_video";

// Bildstorlek — matchar kamerans HQVGA-inställning
#define CAM_W  240
#define CAM_H  176

// Canvas-bufferten MÅSTE ligga i intern SRAM — LVGL använder DMA för att
// skicka pixlar till displayen och DMA kan inte nå PSRAM direkt.
static lv_color_t s_canvas_buf[CAM_W * CAM_H];

// De två ping-pong-buffertarna ligger i PSRAM. De används bara av CPU
// (UDP-montering + JPEG-avkodning), inte DMA, så PSRAM fungerar fint här
// och sparar ~64 KB intern SRAM åt annat.
static uint8_t *s_buf_a;
static uint8_t *s_buf_b;

static uint8_t  *s_asm_buf;   /* UDP-tasken skriver hit just nu */
static uint8_t  *s_dec_buf;   /* avkodaren läser härifrån just nu */
static uint32_t  s_dec_len;
static bool      s_new_frame;
static bool      s_decoding;  /* sann medan avkodaren jobbar — hindrar swap */

static SemaphoreHandle_t s_frame_mutex;

// Kamerans adress lär vi oss dynamiskt från första inkommande UDP-paket.
// s_cam_mutex skyddar läsning/skrivning av adressen mellan de två taskarna.
static int               s_sock      = -1;
static struct sockaddr_in s_cam_addr;
static bool              s_cam_known = false;
static SemaphoreHandle_t s_cam_mutex;

// Tillstånd för pågående bildfragmentering.
// rx_mask är en bitflagga — bit N sätts när fragment N har tagits emot.
// När alla bitar är satta är bilden komplett.
static struct {
    uint16_t seq;
    uint8_t  total_frags;   /* 0 = ingen aktiv bild */
    uint64_t rx_mask;
    uint32_t frame_len;
} s_rx;

// Handler-task: kör LVGL och skickar RC-kommandon till kameran.
// Prioritet 4 — lite högre än UDP-tasken så att UI och styrkommandon
// inte fördröjs av nätverksjobb.
static void handler_task(void *arg)
{
    uint8_t last_cmd = 0xFF;
    while (1) 
    {
        // Driva LVGL — uppdaterar UI-animationer, FPS-räknare, joystick m.m.
        ui_tick();
        lv_timer_handler();

        // Skicka styrbyte till kameran via UDP så fort vi vet dess adress.
        // Vi kopierar adressen under mutex och skickar sedan utan att hålla den.
        if (xSemaphoreTake(s_cam_mutex, 0) == pdTRUE) 
        {
            bool known = s_cam_known;
            struct sockaddr_in addr = s_cam_addr;
            xSemaphoreGive(s_cam_mutex);
            if (known) {
                uint8_t c = ui_get_cmd();
                if (c != last_cmd) last_cmd = c;
                sendto(s_sock, &c, 1, MSG_DONTWAIT,
                       (struct sockaddr *)&addr, sizeof(addr));
            }
        }

        // Kolla om en ny komplett bild finns klar för avkodning.
        // Vi håller mutex bara för att läsa flaggor och pekare — inte under
        // själva JPEG-avkodningen som tar 20–50 ms.
        if (xSemaphoreTake(s_frame_mutex, 0) == pdTRUE) 
        {
            if (s_new_frame) 
            {
                s_new_frame = false;
                s_decoding  = true;
                uint8_t  *src = s_dec_buf;
                uint32_t  len = s_dec_len;
                xSemaphoreGive(s_frame_mutex);  // släpp mutex INNAN avkodning

                int64_t t = esp_timer_get_time();
                esp_jpeg_image_cfg_t cfg = 
                {
                    .indata      = src,
                    .indata_size = len,
                    .outbuf      = (uint8_t *)s_canvas_buf,
                    .outbuf_size = sizeof(s_canvas_buf),
                    .out_format  = JPEG_IMAGE_FORMAT_RGB565,
                    .out_scale   = JPEG_IMAGE_SCALE_0,
                };
                esp_jpeg_image_output_t out;
                esp_err_t err = esp_jpeg_decode(&cfg, &out);
                int64_t ms = (esp_timer_get_time() - t) / 1000;

                // Avkodningen är klar — markera att s_dec_buf nu är fri att swappa
                xSemaphoreTake(s_frame_mutex, portMAX_DELAY);
                s_decoding = false;
                xSemaphoreGive(s_frame_mutex);

                if (err == ESP_OK) 
                {
                    ESP_LOGI(TAG, "Decoded %"PRIu32"x%"PRIu32" in %"PRId64"ms (%"PRIu32"B)", (uint32_t)out.width, (uint32_t)out.height, ms, len);
                    // Märk canvasen som "smutsig" så LVGL ritar om den vid nästa lv_timer_handler()
                    lv_obj_invalidate((lv_obj_t *)arg);
                } else 
                {
                    ESP_LOGE(TAG, "JPEG decode failed: %s", esp_err_to_name(err));
                }
            } else 
            {
                xSemaphoreGive(s_frame_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// UDP-task: tar emot JPEG-fragment och sätter ihop kompletta bilder.
// Prioriteten lägre än handler-tasken men blockerar på recvfrom() och förbrukar ingen CPU när inget data kommer in.
static void udp_video_task(void *arg)
{
    // Lyssna på alla interface, port VID_PORT kameran vet vart den ska skicka
    struct sockaddr_in bind_addr = 
    {
        .sin_family      = AF_INET,
        .sin_port        = htons(VID_PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) 
    {
        ESP_LOGE(TAG, "socket() failed");
        vTaskDelete(NULL);
        return;
    }
    if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) != 0) 
    {
        ESP_LOGE(TAG, "bind() failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    // Öka mottagningsbufferten i kärnan för att klara flera fragment i rad
    // utan att tappa paket om vi är lite sen att läsa
    int rcvbuf = 48 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    s_sock = sock;
    ESP_LOGI(TAG, "UDP video server on port %d", VID_PORT);

    static uint8_t pkt[PKT_MAX];
    uint32_t frames_rx      = 0;
    uint32_t frames_dropped = 0;
    uint32_t fps_count      = 0;
    int64_t  fps_window     = esp_timer_get_time();

    s_rx.total_frags = 0;

    while (1) 
    {
        // Blockera här tills ett paket anländer — förbrukar ingen CPU i väntan
        struct sockaddr_in src;
        socklen_t src_len = sizeof(src);
        int n = recvfrom(sock, pkt, sizeof(pkt), 0, (struct sockaddr *)&src, &src_len);
        if (n < 4) continue;

        // Paketheader: [seq: 2 byte BE] [fragment-index: 1] [totalt antal fragment: 1]
        uint16_t seq;
        memcpy(&seq, pkt, 2);
        seq = ntohs(seq);
        uint8_t fi          = pkt[2];
        uint8_t total_frags = pkt[3];

        if (total_frags == 0 || fi >= total_frags || total_frags > MAX_FRAGS)
            continue;

        // Första paketet vi ser lär oss kamerans adress — vi vet då vart vi ska
        // svara med RC-kommandon (port CMD_PORT, inte VID_PORT)
        if (!s_cam_known) 
        {
            xSemaphoreTake(s_cam_mutex, portMAX_DELAY);
            memcpy(&s_cam_addr, &src, sizeof(src));
            s_cam_addr.sin_port = htons(CMD_PORT);
            s_cam_known = true;
            xSemaphoreGive(s_cam_mutex);
            ui_set_connected(true);
            ESP_LOGI(TAG, "Cam at %s", inet_ntoa(src.sin_addr));
        }

        // Ny sekvens = ny bild. Avbryt föregående om den var ofullständig.
        bool new_seq = (s_rx.total_frags == 0) || (seq != s_rx.seq);
        if (!new_seq && total_frags != s_rx.total_frags) new_seq = true;

        if (new_seq) 
        {
            if (s_rx.total_frags > 0) 
            {
                // Vi hade en pågående bild som inte blev komplett, räkna den som tappad
                uint64_t full = (1ULL << s_rx.total_frags) - 1;
                if (s_rx.rx_mask != full) 
                {
                    frames_dropped++;
                    ESP_LOGW(TAG, "Frame seq %u incomplete (%u/%u frags)", s_rx.seq, (unsigned)__builtin_popcountll(s_rx.rx_mask), s_rx.total_frags);
                }
            }

            // Börja ta emot en ny bild
            s_rx.seq         = seq;
            s_rx.total_frags = total_frags;
            s_rx.rx_mask     = 0;
            s_rx.frame_len   = 0;
        }

        const uint8_t *data = pkt + 4;
        int data_len = n - 4;
        uint32_t offset;

        // Fragment 0 bär en 5-byte header: [magic: 1][total JPEG-längd: 4 BE]
        // Övriga fragment innehåller bara rå JPEG-data, offset räknas ut deterministiskt
        if (fi == 0) 
        {
            if (data_len < 5) continue;
            if (data[0] != FRAME_MAGIC) 
            {
                ESP_LOGE(TAG, "Bad magic: 0x%02x", data[0]);
                continue;
            }
            // Total JPEG-längd i bild 0 — så vi vet när bilden är komplett även om sista fragmentet är mindre än FRAG_SIZE
            uint32_t flen_be;
            memcpy(&flen_be, data + 1, 4);
            s_rx.frame_len = ntohl(flen_be);
            if (s_rx.frame_len > FRAME_BUF) 
            {
                ESP_LOGE(TAG, "Frame too large: %"PRIu32, s_rx.frame_len);
                s_rx.total_frags = 0;
                continue;
            }

            // Data i fragment 0 börjar efter headern så justera pekare och längd
            data     += 5;
            data_len -= 5;
            offset = 0;
        } else 
        {
            // Offset för fragment N: frag 0 fyllde FIRST_DATA bytes, sedan FRAG_SIZE per fragment
            offset = (uint32_t)FIRST_DATA + (uint32_t)(fi - 1) * FRAG_SIZE;
        }

        if ((uint32_t)data_len > FRAME_BUF - offset) continue;
        memcpy(s_asm_buf + offset, data, (size_t)data_len);
        s_rx.rx_mask |= (1ULL << fi);

        // Alla fragment mottagna? Annars väntar vi på fler.
        uint64_t full_mask = (1ULL << total_frags) - 1;
        if (s_rx.rx_mask != full_mask) continue;

        // Komplett bild! Försök swappa buffertar om avkodaren inte jobbar just nu.
        // Om avkodaren är upptagen tappar vi den här bilden — det är okej, nästa bild kommer ändå. 
        frames_rx++;
        fps_count++;

        if (xSemaphoreTake(s_frame_mutex, 0) == pdTRUE) 
        {
            if (!s_decoding) 
            {
                // Byt pekare — snabb swap, ingen kopiering av bilddata
                uint8_t *tmp = s_dec_buf;
                s_dec_buf    = s_asm_buf;
                s_dec_len    = s_rx.frame_len;
                s_asm_buf    = tmp;
                s_new_frame  = true;
            } else 
            {
                frames_dropped++;
                ESP_LOGW(TAG, "Frame dropped (decoder busy) — %"PRIu32"/%"PRIu32, frames_dropped, frames_rx + frames_dropped);
            }
            xSemaphoreGive(s_frame_mutex);
        }

        s_rx.total_frags = 0;

        // Uppdatera FPS-räknaren i UI en gång per sekund
        if (esp_timer_get_time() - fps_window >= 1000000) 
        {
            ui_set_fps(fps_count);
            fps_count  = 0;
            fps_window = esp_timer_get_time();
        }
    }
}

void udp_video_init(void)
{
    // Allokera ping-pong-buffertarna i PSRAM — de behöver inte vara i intern SRAM
    // eftersom varken UDP-mottagning eller JPEG-avkodning använder DMA mot dem
    s_buf_a = heap_caps_malloc(FRAME_BUF, MALLOC_CAP_SPIRAM);
    s_buf_b = heap_caps_malloc(FRAME_BUF, MALLOC_CAP_SPIRAM);
    s_asm_buf = s_buf_a;
    s_dec_buf = s_buf_b;

    s_frame_mutex = xSemaphoreCreateMutex();
    s_cam_mutex   = xSemaphoreCreateMutex();

    // Skapa LVGL-canvasen och koppla den till vår SRAM-pixelbuffert.
    // Videoområdet i UI är x=200..864 (664px), y=36..568 (532px).
    // Vi centrerar 320x240 i det området: screen-offset x=+20, y=+2.
    lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas, s_canvas_buf, CAM_W, CAM_H, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x111111), LV_OPA_COVER);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 20, 2);

    // Starta de två taskarna — handler_task får lite högre prio så att
    // LVGL och styrkommandon aldrig blockeras av nätverksjobb
    xTaskCreate(handler_task,   "lvgl_handler", 8192, canvas, 4, NULL);
    xTaskCreate(udp_video_task, "udp_video",    4096, NULL,   3, NULL);
}
