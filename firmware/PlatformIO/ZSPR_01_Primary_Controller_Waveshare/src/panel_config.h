#pragma once

#include "RGBPanel.h"
#include "hardware_config.h"

// Panel wiring from hardware_config.h, in the form RGBPanel::begin() expects.
inline disp_lcd_panel_config_t makePanelConfig()
{
    return disp_lcd_panel_config_t{
        .width = BOARD_TFT_WIDTH,
        .height = BOARD_TFT_HEIGHT,

        .pclk_freq = RGB_MAX_PIXEL_CLOCK_HZ,
        .pclk_pin = BOARD_TFT_PCLK,
        .hsync_pin = BOARD_TFT_HSYNC,
        .vsync_pin = BOARD_TFT_VSYNC,
        .de_pin = BOARD_TFT_DE,

        .sda_pin = BOARD_MOSI,
        .sck_pin = BOARD_SCLK,
        .cs_pin = -1,  // on the IO expander
        .rst_pin = -1, // on the IO expander
        .bl_pin = BOARD_TFT_BL,

        .r0_pin = BOARD_TFT_R0,
        .r1_pin = BOARD_TFT_R1,
        .r2_pin = BOARD_TFT_R2,
        .r3_pin = BOARD_TFT_R3,
        .r4_pin = BOARD_TFT_R4,
        .r5_pin = BOARD_TFT_R5,

        .g0_pin = BOARD_TFT_G0,
        .g1_pin = BOARD_TFT_G1,
        .g2_pin = BOARD_TFT_G2,
        .g3_pin = BOARD_TFT_G3,
        .g4_pin = BOARD_TFT_G4,
        .g5_pin = BOARD_TFT_G5,

        .b0_pin = BOARD_TFT_B0,
        .b1_pin = BOARD_TFT_B1,
        .b2_pin = BOARD_TFT_B2,
        .b3_pin = BOARD_TFT_B3,
        .b4_pin = BOARD_TFT_B4,
        .b5_pin = BOARD_TFT_B5};
}
