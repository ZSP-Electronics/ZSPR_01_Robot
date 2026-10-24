#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "RGBPanel.h"

// Glue between LVGL, the RGBPanel driver (flush target) and the GT911 touch lib.
// LVGL runs in its own FreeRTOS task; every other task must hold lock() while it
// touches lv_* objects.
class LvglPort
{
public:
    // Requires the panel, I2C bus, IO expander and Touch_Init() to be up already.
    bool begin(RGBPanel &panel);

    static bool lock(uint32_t timeout_ms = portMAX_DELAY);
    static void unlock();

private:
    static void flushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
    static void touchCb(lv_indev_t *indev, lv_indev_data_t *data);
    static void task(void *arg);
};
