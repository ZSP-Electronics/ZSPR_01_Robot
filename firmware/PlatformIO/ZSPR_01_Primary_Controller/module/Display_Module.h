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

// The GT911 reports up to 5 simultaneous contacts.
constexpr uint8_t DISPLAY_MAX_TOUCH_POINTS = 5;

// Property IDs for Display_Module::get/set. The touch props need
// ENABLE_GT911_TOUCH; without it they return NOT_SUPPORTED.
//
// TouchX / TouchY / TouchId are per contact: pack the contact index (0 .. TouchCount-1)
// into the high byte with Display_Module::addr(). Plain TouchX is contact 0. An index at
// or past TouchCount returns INVALID_PARAM.
enum class DisplayProp : uint16_t
{
  Brightness, // get/set UINT  backlight, 0..Backlight_MAX (100)
  Width,      // get UINT  pixels
  Height,     // get UINT  pixels
  TouchReady, // get BOOL  GT911 found at setup()
  Touched,    // get BOOL  at least one finger down
  TouchX,     // get UINT  contact n, pixels
  TouchY,     // get UINT  contact n, pixels
  TouchId,    // get UINT  contact n, the controller's track id (stable while the finger stays down)
  TouchCount, // get UINT  fingers down, 0..DISPLAY_MAX_TOUCH_POINTS
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
  uint8_t id; // which finger: the GT911 track id, the same from Down to Up
};

class Display_Module : public Universal_Module
{
public:
  // Packs a property and a contact index into the 16-bit ID get/set take.
  static constexpr uint16_t addr(DisplayProp prop, uint8_t index) { return (uint16_t)prop | ((uint16_t)index << 8); }

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

        .num_fbs = 2,
        .bounce_lines = RGB_BOUNCE_LINES,

        // .hardware_spi = true,
    };

    /* Initialize Modules */
    bool rslt = panel.begin(&panel_config);
    if (!rslt)
    {
      return ERROR;
    }

    if (!_touch_q)
      _touch_q = xQueueCreate(16, sizeof(TouchEvent)); // up to 5 fingers x a few events per poll

#if ENABLE_GT911_TOUCH
    // Shares the already-initialised Wire bus; reset/IRQ pins sit on the IO expander.
    _touch_ok = touch.begin(BOARD_TOUCH_IRQ, BOARD_TOUCH_RST);
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
      uint8_t n = touch.touched(GT911_MODE_POLLING);
      if (n > DISPLAY_MAX_TOUCH_POINTS)
        n = DISPLAY_MAX_TOUCH_POINTS;

      TouchContact now[DISPLAY_MAX_TOUCH_POINTS];
      for (uint8_t i = 0; i < n; i++)
      {
        GTPoint p = touch.getPoint(i);
        now[i] = {p.trackId, p.x, p.y};
      }

      // Match fingers by track id against the previous poll.
      for (uint8_t i = 0; i < n; i++)
      {
        const TouchContact *before = findContact(_contacts, _touch_count, now[i].id);
        if (!before)
          postTouch(TouchEvent::Down, now[i]);
        else if (before->x != now[i].x || before->y != now[i].y)
          postTouch(TouchEvent::Move, now[i]);
      }
      for (uint8_t j = 0; j < _touch_count; j++)
      {
        if (!findContact(now, n, _contacts[j].id))
          postTouch(TouchEvent::Up, _contacts[j]); // where it lifted
      }

      for (uint8_t i = 0; i < n; i++)
        _contacts[i] = now[i];
      _touch_count = n;
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

  // One finger, as tracked by the controller.
  struct TouchContact
  {
    uint8_t id;
    uint16_t x;
    uint16_t y;
  };

  // Latest touch state, refreshed by update(). Returns false when no finger is down;
  // otherwise the first contact.
  bool getTouch(TouchPoint &pt) const
  {
    DataLock lock(*this);
    if (!_touch_ok || _touch_count == 0)
      return false;
    pt = {_contacts[0].x, _contacts[0].y};
    return true;
  }

  // Copies up to `max` of the fingers currently down into `out`; returns how many.
  uint8_t getTouchPoints(TouchContact *out, uint8_t max) const
  {
    DataLock lock(*this);
    if (!_touch_ok)
      return 0;
    uint8_t n = _touch_count < max ? _touch_count : max;
    for (uint8_t i = 0; i < n; i++)
      out[i] = _contacts[i];
    return n;
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

    uint8_t index = (uint8_t)(id >> 8);
    switch ((DisplayProp)(id & 0xFF))
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
      out = Value(_touch_ok && _touch_count > 0);
      return SUCCESS;
    case DisplayProp::TouchCount:
      out = Value((uint32_t)(_touch_ok ? _touch_count : 0));
      return SUCCESS;
    case DisplayProp::TouchX:
    case DisplayProp::TouchY:
    case DisplayProp::TouchId:
    {
      if (!_touch_ok || index >= _touch_count)
        return INVALID_PARAM;
      const TouchContact &c = _contacts[index];
      DisplayProp prop = (DisplayProp)(id & 0xFF);
      out = Value((uint32_t)(prop == DisplayProp::TouchX ? c.x : prop == DisplayProp::TouchY ? c.y : c.id));
      return SUCCESS;
    }
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
  static const TouchContact *findContact(const TouchContact *list, uint8_t n, uint8_t id)
  {
    for (uint8_t i = 0; i < n; i++)
    {
      if (list[i].id == id)
        return &list[i];
    }
    return nullptr;
  }

  void postTouch(TouchEvent::Type type, const TouchContact &c)
  {
    if (!_touch_q)
      return;
    TouchEvent ev = {type, c.x, c.y, c.id};
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
      return;
    const uint16_t colors[3] = {0xF800, 0x07E0, 0x001F}; // RGB565 red, green, blue
    for (size_t i = 0; i < n; i++)
      buf[i] = colors[(i / _width) * 3 / _height];
    panel.pushColors(0, 0, _width, _height, buf);
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
  TouchContact _contacts[DISPLAY_MAX_TOUCH_POINTS] = {}; // fingers down as of the last poll
  uint8_t _touch_count = 0;
  uint32_t _last_touch_poll = 0;
  uint16_t _touch_poll_ms = 10;
#endif
};