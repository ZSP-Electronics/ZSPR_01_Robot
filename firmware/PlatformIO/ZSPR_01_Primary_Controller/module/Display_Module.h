#pragma once

#include "Universal_Module.h"
#include "hardware_config.h"
#include "board_io.h"
#include "RGBPanel.h"
#include "RGBRoboFace.h"
#include "extra/zsp-electronics-logo.h"

#include <TFT_eSPI.h>

#include "peripheral_config.h"
#if ENABLE_TOUCH
#include <initGT911.h>
#endif

class Display_Module : public Universal_Module
{
public:
  RGBroboFace face;
  TFT_eSprite spr = TFT_eSprite(&tft);

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
          .width = BOARD_TFT_WIDTH,
          .height = BOARD_TFT_HEIGHT,

          .pclk_freq = RGB_MAX_PIXEL_CLOCK_HZ,
          .pclk_pin = BOARD_TFT_PCLK,
          .hsync_pin = BOARD_TFT_HSYNC,
          .vsync_pin = BOARD_TFT_VSYNC,
          .de_pin = BOARD_TFT_DE,

          .sda_pin = BOARD_MOSI,
          .sck_pin = BOARD_SCLK,
          .cs_pin = BOARD_TFT_CS,
          .rst_pin = BOARD_TFT_RST,
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

      /* Initialize Modules */
      bool rslt = panel.begin(&panel_config);
      if (!rslt)
      {
        _enabled = false;
        return ERROR;
      }

      spr.setColorDepth(16);
      if (!spr.createSprite(_width, _height))
      {
        _enabled = false;
        return ERROR;
      }
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

      // face.addMouth(true);
      // face.setCrosshair(true);
      // face.setEyeExpression(Normal);

#if ENABLE_TOUCH
      // Shares the already-initialised Wire bus; reset/IRQ pins sit on the IO expander.
      _touch_ok = touch.begin(BOARD_TOUCH_IRQ, BOARD_TOUCH_RST);
      if (!_touch_ok)
        Serial.println("[display] GT911 touch not found");
#endif
    }
#endif

    uint16_t *redBuffer = nullptr;
    const size_t pixelCount = static_cast<size_t>(BOARD_TFT_WIDTH) * BOARD_TFT_HEIGHT;
    redBuffer = static_cast<uint16_t *>(heap_caps_malloc(
        pixelCount * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    for (size_t pixel = 0; pixel < pixelCount; ++pixel)
    {
      redBuffer[pixel] = 0xF800;
    }

    panel.pushColors(0, 0, BOARD_TFT_WIDTH, BOARD_TFT_HEIGHT, redBuffer);

    return SUCCESS;
  }

  return_codes_t update() override
  {
    if (_enabled)
    {
      if (millis() - _last_refresh >= _target_frame_rate)
      {
        _last_refresh = millis();
        if (draw_robot_face)
          face.update(); // update eyes drawings
        panel.pushColors(0, 0, _width, _height, (uint16_t *)spr.getPointer());
      }

#if ENABLE_TOUCH
      if (_touch_ok && millis() - _last_touch_poll >= _touch_poll_ms)
      {
        _last_touch_poll = millis();
        // Polling mode: the IRQ line is on the IO expander and is not attached.
        _touch_down = touch.touched(GT911_MODE_POLLING) > 0;
        if (_touch_down)
        {
          GTPoint p = touch.getPoint(0);
          _touch_pt = {p.x, p.y};
        }
      }
#endif
    }

    return SUCCESS;
  }

#if ENABLE_TOUCH
  struct TouchPoint
  {
    uint16_t x;
    uint16_t y;
  };

  // Latest touch state, refreshed by update(). Returns false when no finger is down.
  bool getTouch(TouchPoint &pt) const
  {
    if (!_touch_ok || !_touch_down)
      return false;
    pt = _touch_pt;
    return true;
  }

  bool touchReady() const { return _touch_ok; }
#endif

  /**********************/
  /* ALTERNATE FUNCTIONS*/
  /**********************/
  void activate_robot_face()
  {
    if (!_enabled)
      return;
    face.addMouth(true);
    face.setEyeExpression(Normal);
    draw_robot_face = true;
  }

  // Stops animating and blanks the last frame so it doesn't linger under the next screen.
  void deactivate_robot_face()
  {
    draw_robot_face = false;
    if (_enabled)
      spr.fillSprite(TFT_BLACK);
  }

  void draw_splash_screen()
  {
    if (!_enabled)
      return;
    spr.pushImage(0, 0, ZSP_ELECTRONICS_LOGO_WIDTH, ZSP_ELECTRONICS_LOGO_HEIGHT, zsp_electronics_logo);
  }

  // Boot status console: black box over the splash, one line per module.
  void boot_log_begin()
  {
    if (!_enabled)
      return;
    spr.fillRect(kLogX, kLogY, kLogW, kLogH, TFT_BLACK);
    spr.drawRect(kLogX, kLogY, kLogW, kLogH, TFT_DARKGREY);
    _log_line = 0;
    flush();
  }

  // Adds "<name> ........ OK" (green) or "FAIL" (red) and pushes it to the panel.
  void boot_log(const char *name, bool ok)
  {
    if (!_enabled)
      return;
    int16_t y = kLogY + kLogPad + _log_line * kLogLineH;
    if (y + kLogLineH > kLogY + kLogH)
      return; // box full

    spr.setTextFont(4);
    spr.setTextSize(1);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString(name, kLogX + kLogPad, y);

    spr.setTextDatum(TR_DATUM);
    spr.setTextColor(ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
    spr.drawString(ok ? "OK" : "FAIL", kLogX + kLogW - kLogPad, y);
    spr.setTextDatum(TL_DATUM);

    _log_line++;
    flush();
  }

  void boot_log(const char *name, return_codes_t rc) { boot_log(name, rc == SUCCESS); }

#if ENABLE_TOUCH
  void boot_log_touch() { boot_log("Touch", _touch_ok); }
#endif

  // Push the sprite immediately, bypassing the frame-rate limiter.
  void flush()
  {
    if (_enabled)
      panel.pushColors(0, 0, _width, _height, (uint16_t *)spr.getPointer());
  }

private:
#if DISPLAY_RAW_FB_TEST
  // Bring-up check: pushes red/green/blue bands straight to the panel,
  // bypassing the sprite and update(). Dark panel => expander/backlight/init;
  // bands show => problem is in the sprite path. Wrong band colors => RGB order.
  void rawFramebufferTest()
  {
    size_t n = (size_t)_width * _height;
    uint16_t *buf = (uint16_t *)heap_caps_malloc(n * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!buf)
    {
      Serial.println("[display] raw FB test: PSRAM alloc failed");
      return;
    }
    const uint16_t colors[3] = {0xF800, 0x07E0, 0x001F}; // RGB565 red, green, blue
    for (size_t i = 0; i < n; i++)
      buf[i] = colors[(i / _width) * 3 / _height];
    panel.pushColors(0, 0, _width, _height, buf);
    Serial.println("[display] raw FB test: red/green/blue bands pushed");
    delay(3000);
    free(buf);
  }
#endif

  static constexpr int16_t kLogX = 40, kLogY = 120, kLogW = 400, kLogH = 300;
  static constexpr int16_t kLogPad = 16, kLogLineH = 30;
  int16_t _log_line = 0;

  RGBPanel panel;

  TFT_eSPI tft = TFT_eSPI();

  // Not panel.height()/width(): those read panel._disp, which is still null
  // when this member initializer runs (before panel.begin()).
  uint16_t _height = BOARD_TFT_HEIGHT;
  uint16_t _width = BOARD_TFT_WIDTH;
  uint32_t _last_refresh = 0;
  uint16_t _target_frame_rate = 30; // in ms will result in ~33FPS
  bool draw_robot_face = false;

#if ENABLE_TOUCH
  initGT911 touch{&Wire};
  bool _touch_ok = false;
  bool _touch_down = false;
  TouchPoint _touch_pt = {0, 0};
  uint32_t _last_touch_poll = 0;
  uint16_t _touch_poll_ms = 10;
#endif
};