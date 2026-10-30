#pragma once

#include <Arduino.h>

#ifndef BOARD_HAS_PSRAM
#define NO_PSRAM_ERROR
#warning "Please turn on PSRAM to OPI !"
#endif

// #if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(3, 3, 0)
// #error "ESP Arduino Version must be 3.3.0 or less"
// #endif

#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_rgb.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/task.h"

// #include <SD_MMC.h>
#include "Simple_Display.h"
#include "board_io.h"

// constexpr, not #define: names like Resolution/Frequency are far too generic
// to be macros -- a macro here silently rewrites any enum member or variable
// of the same name in every file that includes this header (it broke
// TofProp::Resolution).
constexpr uint8_t PWM_Channel = 1;    // PWM Channel
constexpr uint32_t Frequency = 20000; // PWM frequency
constexpr uint8_t Resolution = 10;    // PWM resolution ratio     MAX:13
constexpr uint16_t Dutyfactor = 500;  // PWM Dutyfactor
constexpr uint8_t Backlight_MAX = 100;

enum RGBPanel_Color_Order
{
    RGB_ORDER_RGB,
    RGB_ORDER_BGR,
};

struct disp_lcd_panel_config_t
{
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

    // bool hardware_spi;

    // Buffering (ESP-IDF 5.x RGB driver). 2 frame buffers + vsync swap give tear-free output;
    // begin() falls back to 1 if the driver cannot allocate/run with 2.
    uint8_t num_fbs = 2;
    // Height of the internal-RAM bounce buffer in lines (width*height must divide evenly by it).
    uint16_t bounce_lines = 20;
};

class RGBPanel : public Simple_Display
{

public:
    RGBPanel();
    ~RGBPanel();

    bool begin(disp_lcd_panel_config_t *disp, RGBPanel_Color_Order order = RGB_ORDER_RGB, bool IO_Init = false);

    void setBrightness(uint8_t level);
    uint8_t getBrightness();

    uint16_t width();
    uint16_t height();

    void pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *data);

    // Frame buffers owned by the RGB driver (1 or 2, see disp_lcd_panel_config_t::num_fbs).
    uint8_t frameBufferCount() const { return _numFbs; }
    uint16_t *frameBuffer(uint8_t i); // nullptr if i >= frameBufferCount()

    // Double-buffer path: switch scan-out to `fb` (one of frameBuffer()) at the next vsync and wait for it.
    void showFrameBuffer(uint16_t *fb, uint32_t timeout_ms = 100);

private:
    void writeData(uint8_t data);
    void writeData(const uint8_t *data, int len);
    void writeCommand(const uint8_t cmd);
    void releaseBus();

    void Panel_CS_EN()
    {
        board_digitalWrite(_disp->cs_pin, LOW);
        // vTaskDelay(pdMS_TO_TICKS(10));
        delay(10);
    }
    void Panel_CS_DIS()
    {
        board_digitalWrite(_disp->cs_pin, HIGH);
        // vTaskDelay(pdMS_TO_TICKS(10));
        delay(10);
    }
    void Panel_Reset()
    {
        board_digitalWrite(_disp->rst_pin, LOW);
        // vTaskDelay(pdMS_TO_TICKS(10));
        delay(10);
        board_digitalWrite(_disp->rst_pin, HIGH);
        // vTaskDelay(pdMS_TO_TICKS(50));
        delay(50);
    }

    uint8_t _brightness;

    esp_lcd_panel_handle_t _panelDrv;
    RGBPanel_Color_Order _order;
    disp_lcd_panel_config_t *_disp;
    // spi_device_handle_t SPI_handle = NULL;

    bool _has_init;
    bool _IO_Init;

    uint8_t _numFbs = 0;
    uint16_t *_fbs[2] = {nullptr, nullptr};
    SemaphoreHandle_t _vsync = nullptr;
};
