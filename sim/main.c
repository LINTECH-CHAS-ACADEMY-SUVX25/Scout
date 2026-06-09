// Scout UI-simulator — kör samma LVGL-UI som skärmen på en PC via SDL2.
//
// Den här filen ersätter driver-delen av lvgl_port.c (flush, touch, tick,
// init, render-loop) med SDL och är ENBART för simulatorn. UI:t självt
// byggs av sim_ui_init() i ui.c, som speglar enhetskoden.

#include "lvgl.h"
#include "sim_screen.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

// Kamerarutans mått i den kommande designen. På enheten blittas videon
// rakt in i framebuffern (förbi LVGL); här ritar vi bara en referensram
// så layouten kan ritas runt rätt yta.
#define CAM_W  480
#define CAM_H  480
#define CAM_X  ((SCREEN_W - CAM_W) / 2)
#define CAM_Y  ((SCREEN_H - CAM_H) / 2)

// UI byggd i ui.c
void sim_ui_init(void);
void lvgl_port_ui_update(bool connected);
void lvgl_port_intro_screen(void);

static SDL_Window   *s_win;
static SDL_Renderer *s_ren;
static SDL_Texture  *s_tex;

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    SDL_Rect r = { area->x1, area->y1, w, h };
    SDL_UpdateTexture(s_tex, &r, color_p, w * sizeof(lv_color_t));

    if(lv_disp_flush_is_last(drv)) {
        SDL_RenderClear(s_ren);
        SDL_RenderCopy(s_ren, s_tex, NULL, NULL);
        SDL_RenderPresent(s_ren);
    }
    lv_disp_flush_ready(drv);
}

static void mouse_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    int x, y;
    uint32_t buttons = SDL_GetMouseState(&x, &y);
    data->point.x = x;
    data->point.y = y;
    data->state   = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
                        ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// Referensram för kamerarutan — finns bara i simulatorn.
static void make_camera_box(void)
{
    lv_obj_t *box = lv_obj_create(lv_scr_act());
    lv_obj_set_size(box, CAM_W, CAM_H);
    lv_obj_set_pos(box, CAM_X, CAM_Y);
    lv_obj_set_style_radius(box, 0, 0);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x0A0E14), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0x22D3EE), 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *l = lv_label_create(box);
    lv_label_set_text(l, "CAMERA\n480 x 480");
    lv_obj_set_style_text_color(l, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
}

static void sdl_init(void)
{
    SDL_Init(SDL_INIT_VIDEO);
    s_win = SDL_CreateWindow("Scout UI sim",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, 0);
    s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_ACCELERATED);
    s_tex = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STATIC, SCREEN_W, SCREEN_H);
}

static void lvgl_init(void)
{
    lv_init();

    static lv_color_t buf[SCREEN_W * 100];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_W * 100);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res      = SCREEN_W;
    disp_drv.ver_res      = SCREEN_H;
    disp_drv.flush_cb     = flush_cb;
    disp_drv.draw_buf     = &draw_buf;
    disp_drv.full_refresh = 0;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mouse_read_cb;
    lv_indev_drv_register(&indev_drv);
}

// Renderar några frames och sparar en BMP. Kör med SIM_SHOT=fil.bmp ./sim
// (oftast med SDL_VIDEODRIVER=offscreen) för att fånga UI:t utan fönster.
static void screenshot(const char *path)
{
    const char *ms = SDL_getenv("SIM_SHOT_MS");   // spola fram så här långt först
    int budget = ms ? SDL_atoi(ms) : 100;
    for(int t = 0; t < budget; t += 20) {
        lv_tick_inc(20);
        lv_timer_handler();
    }
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, SCREEN_W, SCREEN_H, 16, SDL_PIXELFORMAT_RGB565);
    SDL_RenderReadPixels(s_ren, NULL, SDL_PIXELFORMAT_RGB565,
        surf->pixels, surf->pitch);
    SDL_SaveBMP(surf, path);
    SDL_FreeSurface(surf);
}

int main(void)
{
    sdl_init();
    lvgl_init();

    sim_ui_init();
    make_camera_box();
    lvgl_port_intro_screen();

    const char *shot = SDL_getenv("SIM_SHOT");
    if(shot) {
        screenshot(shot);
        SDL_Quit();
        return 0;
    }

    bool running   = true;
    bool connected = false;
    uint32_t last  = SDL_GetTicks();

    while(running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) {
                running = false;
            } else if(e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if(k == SDLK_ESCAPE || k == SDLK_q) {
                    running = false;
                } else if(k == SDLK_c) {
                    connected = !connected;       // växla connected-state
                    lvgl_port_ui_update(connected);
                }
            }
        }

        uint32_t now = SDL_GetTicks();
        lv_tick_inc(now - last);
        last = now;
        lv_timer_handler();
        SDL_Delay(5);
    }

    SDL_DestroyTexture(s_tex);
    SDL_DestroyRenderer(s_ren);
    SDL_DestroyWindow(s_win);
    SDL_Quit();
    return 0;
}
