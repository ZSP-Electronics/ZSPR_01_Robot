/**
 * LVGL 9.2 configuration for the Waveshare ESP32-S3-Touch-LCD-2.8C
 * (480x480 RGB565 parallel panel, 8 MB PSRAM, Arduino-ESP32, FreeRTOS).
 *
 * Only options that differ from LVGL's defaults are set here; everything else
 * falls back to lv_conf_internal.h. Found via -DLV_CONF_INCLUDE_SIMPLE + -Iinclude.
 */

#if 1 /* Set this to "1" to enable content */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/
/* Panel is driven as RGB565; no byte swap needed for the parallel RGB bus. */
#define LV_COLOR_DEPTH 16

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/
/* Use the system heap (internal RAM under Arduino-ESP32; widgets are small). Large buffers
 * such as frame/draw buffers are allocated explicitly with heap_caps_malloc(). */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

/*====================
   HAL SETTINGS
 *====================*/
/* ~60 fps refresh timer. Tick comes from lv_tick_set_cb() in lvgl_port.cpp. */
#define LV_DEF_REFR_PERIOD  16
#define LV_DPI_DEF          160

/*=================
 * OPERATING SYSTEM
 *=================*/
/* LVGL is not thread safe; locking is done by lvgl_port_lock()/unlock() instead. */
#define LV_USE_OS   LV_OS_NONE

/*========================
 * RENDERING CONFIGURATION
 *========================*/
#define LV_DRAW_BUF_STRIDE_ALIGN        1
#define LV_DRAW_BUF_ALIGN               64     /* matches the S3 64 B data cache line */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE   (64 * 1024)

#define LV_USE_DRAW_SW  1
#if LV_USE_DRAW_SW == 1
    #define LV_DRAW_SW_SUPPORT_RGB565       1
    #define LV_DRAW_SW_SUPPORT_RGB565A8     0
    #define LV_DRAW_SW_SUPPORT_RGB888       1
    #define LV_DRAW_SW_SUPPORT_XRGB8888     1
    #define LV_DRAW_SW_SUPPORT_ARGB8888     1
    #define LV_DRAW_SW_SUPPORT_L8           0
    #define LV_DRAW_SW_SUPPORT_AL88         0
    #define LV_DRAW_SW_SUPPORT_A8           1
    #define LV_DRAW_SW_SUPPORT_I1           0
    #define LV_DRAW_SW_DRAW_UNIT_CNT        1   /* one render thread; second core is for the robot */
    #define LV_DRAW_SW_COMPLEX              1   /* rounded corners, shadows, gradients: needed for the face */
    #define LV_USE_DRAW_SW_ASM              LV_DRAW_SW_ASM_NONE
#endif

/*=================
 * LOGGING
 *=================*/
#define LV_USE_LOG  1
#if LV_USE_LOG
    #define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF   1
#endif

/*=================
 * ASSERTS
 *=================*/
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*=================
 * FONTS
 *=================*/
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_MONTSERRAT_40   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_20

/*=================
 * THEME
 *=================*/
#define LV_USE_THEME_DEFAULT    1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK           1
    #define LV_THEME_DEFAULT_GROW           1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

/*=================
 * WIDGETS
 *=================*/
/* Button, label, slider, switch, textarea, keyboard, list, arc, etc. all stay at
 * LVGL's defaults (enabled). Turn unused ones off here to save flash. */

/*=================
 * DEBUG / PROFILING
 *=================*/
/* On-screen FPS + CPU load. Useful while tuning the pixel clock; set both to 0 for release. */
#define LV_USE_SYSMON       1
#if LV_USE_SYSMON
    #define LV_USE_PERF_MONITOR     1
    #define LV_USE_MEM_MONITOR      0
#endif

#endif /*LV_CONF_H*/

#endif /*End of "Content enable"*/
