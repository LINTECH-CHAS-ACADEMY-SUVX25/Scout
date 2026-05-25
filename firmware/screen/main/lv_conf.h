#if 1  /* Set this to 1 to enable lv_conf.h */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16

#define LV_HOR_RES_MAX 1024
#define LV_VER_RES_MAX 600

#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64 * 1024U)

#define LV_USE_BTN   1
#define LV_USE_LABEL 1
#define LV_USE_LOG   1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

#endif /* LV_CONF_H */
#endif /* Enable lv_conf.h */
