#pragma once

#include "Universal_Module.h"
#include "hardware_config.h"
#include "board_io.h"
#include "RGBPanel.h"
#include "RGBRoboFace.h"
#include "protocol_handlers.h"

#include <TFT_eSPI.h>

class Display_Module : public Universal_Module
{
public:
  Display_Module(bool enable) : Universal_Module(enable) {}

  return_codes_t setup() override
  {
    if (_enabled)
    {
#ifdef NO_PSRAM_ERROR
      _enabled = false;
      return NOT_SUPPORTED;
    }
#else
      disp_lcd_panel_config_t panel_config = {
          .width = 480,
          .height = 480,

          .pclk_freq = 8000000UL,
          .pclk_pin = BOARD_TFT_PCLK,
          .hsync_pin = BOARD_TFT_HSYNC,
          .vsync_pin = BOARD_TFT_VSYNC,
          .de_pin = BOARD_TFT_DE,

          .sda_pin = BOARD_MOSI,
          .sck_pin = BOARD_SCLK,
          .cs_pin = BOARD_TFT_CS,
          .rst_pin = BOARD_TFT_RST,
          .bl_pin = BOARD_TFT_BL,

          .r0_pin = BOARD_TFT_DATA0,
          .r1_pin = BOARD_TFT_DATA1,
          .r2_pin = BOARD_TFT_DATA2,
          .r3_pin = BOARD_TFT_DATA3,
          .r4_pin = BOARD_TFT_DATA4,
          .r5_pin = BOARD_TFT_DATA5,

          .g0_pin = BOARD_TFT_DATA6,
          .g1_pin = BOARD_TFT_DATA7,
          .g2_pin = BOARD_TFT_DATA8,
          .g3_pin = BOARD_TFT_DATA9,
          .g4_pin = BOARD_TFT_DATA10,
          .g5_pin = BOARD_TFT_DATA11,

          .b0_pin = BOARD_TFT_DATA12,
          .b1_pin = BOARD_TFT_DATA13,
          .b2_pin = BOARD_TFT_DATA14,
          .b3_pin = BOARD_TFT_DATA15,
          .b4_pin = BOARD_TFT_DATA16,
          .b5_pin = BOARD_TFT_DATA17};

      /* Initialize Modules */
      bool rslt = panel.begin(&panel_config, RGB_ORDER_RGB);
      if (!rslt)
      {
        while (1)
        {
          Serial.println("Error, failed to initialize Display");
          delay(1000);
        }
      }

      spr.createSprite(_width, _height);
      spr.setColorDepth(16);
      spr.setSwapBytes(0);

      face.begin(&spr);
      // face.setEyeHeight(100, 150);
      // face.setEyeWidth(150, 75);
      // face.setEyeBorderradius(25);
      // face.setEyeSpacebetween(-10);
      // face.setAutoblinker(true);
      // face.setCuriosity(true);
      // face.setCuriosityOffsets(0, -20);
      // face.setPositionDistance(25);

      face.addMouth(true);
      // face.setCrosshair(true);
      face.setEyeExpression(Normal);
    }
#endif
return SUCCESS;
    }

    return_codes_t update() override
    {
      if (_enabled)
      {
        if (millis() - _last_refresh >= _target_frame_rate)
        {
          _last_refresh = millis();
          face.update(); // update eyes drawings
          panel.pushColors(0, 0, _width, _height, (uint16_t *)spr.getPointer());
        }
      }

      return SUCCESS;
    }

    return_codes_t getData(uint16_t *data, int argc, char **argv) override { return SUCCESS; }
    return_codes_t setData(uint16_t *data, int argc, char **argv) override { return SUCCESS; }

  private:
    RGBPanel panel;
    RGBroboFace face;
    TFT_eSPI tft = TFT_eSPI();
    TFT_eSprite spr = TFT_eSprite(&tft);

    uint16_t _height = panel.height();
    uint16_t _width = panel.width();
    uint32_t _last_refresh = 0;
    uint16_t _target_frame_rate = 30; // in ms will result in ~33FPS
  };