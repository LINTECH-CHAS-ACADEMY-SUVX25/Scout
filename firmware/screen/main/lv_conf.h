#if 1  // Set to 1 to enable this config, 0 to disable and use the default one.

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// Display resolution
#define LV_COLOR_DEPTH      16
#define LV_HOR_RES_MAX      1024
#define LV_VER_RES_MAX      600

// Memory management
#define LV_MEM_CUSTOM       0
#define LV_MEM_SIZE         (96 * 1024U)   

// Feature usage
#define LV_USE_BTN          1
#define LV_USE_LABEL        1
#define LV_USE_ARC          1   

// Transform, krävs för knobens zoom-effekt
#define LV_USE_OBJ_TRANSFORM   1  

// Fonts
#define LV_FONT_MONTSERRAT_14  1  

// Logging
#define LV_USE_LOG          1
#define LV_LOG_LEVEL        LV_LOG_LEVEL_WARN

#endif 
#endif 