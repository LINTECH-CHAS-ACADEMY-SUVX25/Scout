/**
 * @file lv_conf.h
 * @brief LVGL config for Scout Simulator – Waveshare 7" 1024×600
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH          16
#define LV_HOR_RES_MAX          1024
#define LV_VER_RES_MAX          600
#define LV_MEM_SIZE             (128U * 1024U)
#define LV_DISP_DEF_REFR_PERIOD 16       /* ~60 fps */

/* Fonts */
#define LV_FONT_MONTSERRAT_8    0
#define LV_FONT_MONTSERRAT_10   1
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   0
#define LV_FONT_MONTSERRAT_20   0
#define LV_FONT_MONTSERRAT_22   0
#define LV_FONT_MONTSERRAT_24   0
#define LV_FONT_MONTSERRAT_26   0
#define LV_FONT_MONTSERRAT_28   0
#define LV_FONT_MONTSERRAT_30   0
#define LV_FONT_MONTSERRAT_32   0
#define LV_FONT_MONTSERRAT_34   0
#define LV_FONT_MONTSERRAT_36   0
#define LV_FONT_MONTSERRAT_38   0
#define LV_FONT_MONTSERRAT_40   0
#define LV_FONT_MONTSERRAT_42   0
#define LV_FONT_MONTSERRAT_44   0
#define LV_FONT_MONTSERRAT_46   0
#define LV_FONT_MONTSERRAT_48   0
#define LV_FONT_DEFAULT         &lv_font_montserrat_12

/* Widgets */
#define LV_USE_LABEL   1
#define LV_USE_OBJ     1
#define LV_USE_CANVAS  1
#define LV_USE_BAR     1
#define LV_USE_BTN     1
#define LV_USE_LINE    1
#define LV_USE_ARC     1

/* Logging */
#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN

/* SDL driver */
#define USE_SDL              1
#define SDL_HOR_RES          1024
#define SDL_VER_RES          600
#define SDL_ZOOM             1
#define SDL_DOUBLE_BUFFERED  0

#endif /* LV_CONF_H */
