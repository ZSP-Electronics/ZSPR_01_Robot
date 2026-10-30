#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Universal_Module.h"
#include "hardware_config.h"
#include "board_io.h"
#include "RGBPanel.h"

#include "peripheral_config.h"
#if ENABLE_GT911_TOUCH
#include <initGT911.h>
#endif

// Property IDs for Display_Module::get/set. The touch props need
// ENABLE_GT911_TOUCH; without it they return NOT_SUPPORTED.
enum class DisplayProp : uint16_t
{
  Brightness, // get/set UINT  backlight, 0..Backlight_MAX (100)
  Width,      // get UINT  pixels
  Height,     // get UINT  pixels
  TouchReady, // get BOOL  GT911 found at setup()
  Touched,    // get BOOL  finger currently down
  TouchX,     // get UINT  last touch point
  TouchY,     // get UINT
};

// One touch change, posted to touchQueue() by update(). Move is only sent when
// the point actually moved. Always defined so UI code compiles without the
// GT911; the queue just never receives anything then.
struct TouchEvent
{
  enum Type : uint8_t
  {
    Down,
    Move,
    Up
  };
  Type type;
  uint16_t x;
  uint16_t y;
};

class Display_Module : public Universal_Module
{
public:
  // I2C only: the GT911 touch controller uses Wire. The panel's SPI is just the
  // bit-banged init in setup(), which runs before any task starts.
  Display_Module(bool enable) : Universal_Module(enable) { _buses = BUS_MASK_I2C; }

  return_codes_t setup() override
  {
    if (!_enabled)
      return ERROR;

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
        .b5_pin = BOARD_TFT_B5,

        // .hardware_spi = true,
    };

    /* Initialize Modules */
    bool rslt = panel.begin(&panel_config);
    if (!rslt)
    {
      return ERROR;
    }

    if (!_touch_q)
      _touch_q = xQueueCreate(8, sizeof(TouchEvent));

#if ENABLE_GT911_TOUCH
    // Shares the already-initialised Wire bus; reset/IRQ pins sit on the IO expander.
    _touch_ok = touch.begin(BOARD_TOUCH_IRQ, BOARD_TOUCH_RST);
    if (!_touch_ok)
      Serial.println("[display] GT911 touch not found");
#endif

    _initialized = true;
    return SUCCESS;
  }

  return_codes_t update() override
  {
    if (!_enabled || !_initialized)
      return ERROR;

#if ENABLE_GT911_TOUCH
    if (_touch_ok && millis() - _last_touch_poll >= _touch_poll_ms)
    {
      _last_touch_poll = millis();
      // Polling mode: the IRQ line is on the IO expander and is not attached.
      bool down = touch.touched(GT911_MODE_POLLING) > 0;
      TouchPoint pt = _touch_pt;
      if (down)
      {
        GTPoint p = touch.getPoint(0);
        pt = {p.x, p.y};
      }

      if (down && !_touch_down)
        postTouch(TouchEvent::Down, pt.x, pt.y);
      else if (down && (pt.x != _touch_pt.x || pt.y != _touch_pt.y))
        postTouch(TouchEvent::Move, pt.x, pt.y);
      else if (!down && _touch_down)
        postTouch(TouchEvent::Up, _touch_pt.x, _touch_pt.y); // where it lifted

      _touch_down = down;
      _touch_pt = pt;
    }
#endif

    return SUCCESS;
  }

  // Draws a rectangle of RGB565 pixels. Goes through the module lock so a
  // second task drawing (or the CLI changing brightness) can't interleave with
  // it on the panel.
  bool push_colors(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data)
  {
    DataLock lock(*this);
    if (!_enabled || !_initialized)
      return false;
    panel.pushColors(x, y, w, h, data);
    return true;
  }

  // Queue of TouchEvent, filled by update() (null until setup() succeeds).
  // Single consumer: the UI task.
  QueueHandle_t touchQueue() const { return _touch_q; }

  // Double-buffered drawing for the single UI task -- not locked, only one
  // task may call these. backBuffer() is the frame buffer that is not being
  // scanned out; draw into it, then present() to show it. With only one frame
  // buffer (the driver's fallback) backBuffer() is the live one: drawing tears
  // and present() does nothing.
  uint8_t frameBufferCount()
  {
    return _initialized ? panel.frameBufferCount() : 0;
  }

  uint16_t *backBuffer()
  {
    if (!_initialized)
      return nullptr;
    return panel.frameBuffer(panel.frameBufferCount() > 1 ? 1 - _front_fb : 0);
  }

  // Both buffers, for clearing them at start-up.
  uint16_t *frameBuffer(uint8_t i) { return _initialized ? panel.frameBuffer(i) : nullptr; }

  bool present()
  {
    if (!_initialized)
      return false;
    if (panel.frameBufferCount() < 2)
      return true;
    uint8_t back = 1 - _front_fb;
    panel.showFrameBuffer(panel.frameBuffer(back)); // switches at vsync and waits
    _front_fb = back;
    return true;
  }

#if ENABLE_GT911_TOUCH
  struct TouchPoint
  {
    uint16_t x;
    uint16_t y;
  };

  // Latest touch state, refreshed by update(). Returns false when no finger is down.
  bool getTouch(TouchPoint &pt) const
  {
    DataLock lock(*this);
    if (!_touch_ok || !_touch_down)
      return false;
    pt = _touch_pt;
    return true;
  }

  bool touchReady() const
  {
    DataLock lock(*this);
    return _touch_ok;
  }
#endif

protected:
  return_codes_t onGet(uint16_t id, Value &out) override
  {
    if (!_initialized)
      return ERROR;

    switch ((DisplayProp)id)
    {
    case DisplayProp::Brightness:
      out = Value((uint32_t)panel.getBrightness());
      return SUCCESS;
    case DisplayProp::Width:
      out = Value((uint32_t)_width);
      return SUCCESS;
    case DisplayProp::Height:
      out = Value((uint32_t)_height);
      return SUCCESS;
#if ENABLE_GT911_TOUCH
    case DisplayProp::TouchReady:
      out = Value(_touch_ok);
      return SUCCESS;
    case DisplayProp::Touched:
      out = Value(_touch_ok && _touch_down);
      return SUCCESS;
    case DisplayProp::TouchX:
      out = Value((uint32_t)_touch_pt.x);
      return SUCCESS;
    case DisplayProp::TouchY:
      out = Value((uint32_t)_touch_pt.y);
      return SUCCESS;
#endif
    default:
      return NOT_SUPPORTED;
    }
  }

  return_codes_t onSet(uint16_t id, const Value &in) override
  {
    if (!_initialized)
      return ERROR;

    switch ((DisplayProp)id)
    {
    case DisplayProp::Brightness:
      panel.setBrightness((uint8_t)constrain(in.asInt(), 0, (int)Backlight_MAX));
      return SUCCESS;
    default:
      return NOT_SUPPORTED;
    }
  }

private:
  QueueHandle_t _touch_q = nullptr;
  uint8_t _front_fb = 0; // index of the frame buffer being scanned out

#if ENABLE_GT911_TOUCH
  // Never blocks: update() runs under the module lock. Move events are
  // droppable; Down/Up are not, so they push out the oldest entry instead.
  void postTouch(TouchEvent::Type type, uint16_t x, uint16_t y)
  {
    if (!_touch_q)
      return;
    TouchEvent ev = {type, x, y};
    if (xQueueSend(_touch_q, &ev, 0) != pdTRUE && type != TouchEvent::Move)
    {
      TouchEvent dropped;
      xQueueReceive(_touch_q, &dropped, 0);
      xQueueSend(_touch_q, &ev, 0);
    }
  }
#endif

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

  // Not panel.height()/width(): those read panel._disp, which is still null
  // when this member initializer runs (before panel.begin()).
  uint16_t _height = BOARD_TFT_HEIGHT;
  uint16_t _width = BOARD_TFT_WIDTH;
  uint32_t _last_refresh = 0;
  uint16_t _target_frame_rate = 30; // in ms will result in ~33FPS
  bool draw_robot_face = false;

#if ENABLE_GT911_TOUCH
  initGT911 touch{&Wire};
  bool _touch_ok = false;
  bool _touch_down = false;
  TouchPoint _touch_pt = {0, 0};
  uint32_t _last_touch_poll = 0;
  uint16_t _touch_poll_ms = 10;
#endif
};