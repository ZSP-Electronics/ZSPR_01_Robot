#pragma once

#include <Arduino.h>

#ifndef BOARD_HAS_PSRAM
#define NO_PSRAM_ERROR
#warning "Please turn on PSRAM to OPI !"
#endif

// #if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(3, 3, 0)
// #error "ESP Arduino Version must be 3.3.0 or less"
// #endif

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>

// #include <SD_MMC.h>
#include "Simple_Display.h"
#include "board_io.h"

enum RGBPanel_Color_Order {
    RGB_ORDER_RGB,
    RGB_ORDER_BGR,
};

struct disp_lcd_panel_config_t {
    uint16_t width;
    uint16_t height;

    uint32_t pclk_freq;
    int pclk_pin;
    int hsync_pin;
    int vsync_pin;
    int de_pin;

    int sda_pin;
    int sck_pin;
    int cs_pin;
    int rst_pin;
    int bl_pin;

    int r0_pin;
    int r1_pin;
    int r2_pin;
    int r3_pin;
    int r4_pin;
    int r5_pin;
    int g0_pin;
    int g1_pin;
    int g2_pin;
    int g3_pin;
    int g4_pin;
    int g5_pin;
    int b0_pin;
    int b1_pin;
    int b2_pin;
    int b3_pin;
    int b4_pin;
    int b5_pin;

    bool use_wire;
    bool use_spi;
};

class RGBPanel : public Simple_Display
{

public:
    RGBPanel();

    ~RGBPanel();

    bool begin(disp_lcd_panel_config_t *disp, RGBPanel_Color_Order order = RGB_ORDER_RGB, bool IO_Init = false);

    void setBrightness(uint8_t level);
    uint8_t getBrightness();

    uint16_t  width();
    uint16_t  height();

    void pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *data);

private:

    void writeData(const uint8_t *data, int len);

    void writeCommand(const uint8_t cmd);

    void initBUS();
    void initDevice();

    uint8_t _brightness;

    esp_lcd_panel_handle_t _panelDrv;
    RGBPanel_Color_Order  _order;
    disp_lcd_panel_config_t* _disp;

    bool _has_init;
    bool _IO_Init;

};



