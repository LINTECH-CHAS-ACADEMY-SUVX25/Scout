/**
 * @file lv_drv_conf.h
 * @brief LVGL Drivers config for Scout Simulator – SDL backend
 */

#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H

#include "lv_conf.h"

/*********************
 * DELAY INTERFACE
 *********************/
#define LV_DRV_DELAY_INCLUDE  <stdint.h>
#define LV_DRV_DELAY_US(us)   /*delay_us(us)*/
#define LV_DRV_DELAY_MS(ms)   /*delay_ms(ms)*/

/*********************
 * DISPLAY INTERFACE
 *********************/

/*------------
 *  Common
 *------------*/
#define LV_DRV_DISP_INCLUDE         <stdint.h>
#define LV_DRV_DISP_CMD_DATA(val)   /*pin_x_set(val)*/
#define LV_DRV_DISP_RST(val)        /*pin_x_set(val)*/

/*---------
 *  SPI
 *---------*/
#define LV_DRV_DISP_SPI_CS(val)          /*spi_cs_set(val)*/
#define LV_DRV_DISP_SPI_WR_BYTE(data)    /*spi_wr(data)*/
#define LV_DRV_DISP_SPI_WR_ARRAY(adr, n) /*spi_wr_mem(adr, n)*/

/*------------------
 *  Parallel port
 *-----------------*/
#define LV_DRV_DISP_PAR_CS(val)          /*par_cs_set(val)*/
#define LV_DRV_DISP_PAR_SLOW             /*par_slow()*/
#define LV_DRV_DISP_PAR_FAST             /*par_fast()*/
#define LV_DRV_DISP_PAR_WR_WORD(data)    /*par_wr(data)*/
#define LV_DRV_DISP_PAR_WR_ARRAY(adr, n) /*par_wr_mem(adr,n)*/

/***************************
 * INPUT DEVICE INTERFACE
 ***************************/

/*----------
 *  Common
 *----------*/
#define LV_DRV_INDEV_INCLUDE     <stdint.h>
#define LV_DRV_INDEV_RST(val)    /*pin_x_set(val)*/
#define LV_DRV_INDEV_IRQ_READ    0 /*pn_x_read()*/

/*---------
 *  SPI
 *---------*/
#define LV_DRV_INDEV_SPI_CS(val)            /*spi_cs_set(val)*/
#define LV_DRV_INDEV_SPI_XCHG_BYTE(data)    0 /*spi_xchg(val)*/

/*---------
 *  I2C
 *---------*/
#define LV_DRV_INDEV_I2C_START              /*i2c_start()*/
#define LV_DRV_INDEV_I2C_STOP               /*i2c_stop()*/
#define LV_DRV_INDEV_I2C_RESTART            /*i2c_restart()*/
#define LV_DRV_INDEV_I2C_WR(data)           /*i2c_wr(data)*/
#define LV_DRV_INDEV_I2C_READ(last_read)    0 /*i2c_rd()*/


/*********************
 *  DISPLAY DRIVERS
 *********************/

/*-------------------
 *  SDL
 *-------------------*/

/* SDL based drivers for display, mouse, mousewheel and keyboard*/
#define USE_SDL 1

/* Hardware accelerated SDL driver */
#define USE_SDL_GPU 0

#if USE_SDL || USE_SDL_GPU
#  define SDL_HOR_RES     1024
#  define SDL_VER_RES     600

/* Scale window by this factor (useful when simulating small screens) */
#  define SDL_ZOOM        1

/* Used to test true double buffering with only address changing.
 * Use 2 draw buffers, bith with SDL_HOR_RES x SDL_VER_RES size*/
#  define SDL_DOUBLE_BUFFERED 0

/*Eclipse: <SDL2/SDL.h>    Visual Studio: <SDL.h>*/
#  define SDL_INCLUDE_PATH    <SDL2/SDL.h>

/*Open two windows to test multi display support*/
#  define SDL_DUAL_DISPLAY            0
#endif

/*-------------------
 *  Monitor of PC
 *-------------------*/
#define USE_MONITOR         0

/*-------------------
 *  GTK on Linux
 *-------------------*/
#define USE_GTK             0

/*------------------
 *  Windows (WIN32)
 *------------------*/
#define USE_WINDOWS         0

/*---------------------------
 *  Native Linux frame buffer
 *---------------------------*/
#define USE_FBDEV           0

/*---------------------------
 *  Native Linux DRM/KMS
 *---------------------------*/
#define USE_DRM             0

/*********************
 *  INPUT DEVICES
 *********************/

/*---------------------------------------
 * Mouse or touchpad on PC (using SDL)
 *-------------------------------------*/
/* Already handled by USE_SDL */

/*-------------------------------------------
 * Mousewheel as encoder on PC (using SDL)
 *------------------------------------------*/
/* Already handled by USE_SDL */

/*-------------------------------
 *   Keyboard of a PC (using SDL)
 *------------------------------*/
/* Already handled by USE_SDL */

/*-------------------------------
 *  Touchscreen via evdev
 *------------------------------*/
#define USE_EVDEV           0
#define USE_BSD_EVDEV       0

/*-------------------------------
 *  Touchscreen via libinput
 *------------------------------*/
#define USE_LIBINPUT        0
#define USE_BSD_LIBINPUT    0

/*-------------------------------
 *  Mouse via XKB
 *------------------------------*/
#define USE_XKB             0

/*-------------------------------
 *  Keyboard via libinput
 *------------------------------*/
#define USE_LIBINPUT_FULL_KB  0

#endif  /* LV_DRV_CONF_H */
