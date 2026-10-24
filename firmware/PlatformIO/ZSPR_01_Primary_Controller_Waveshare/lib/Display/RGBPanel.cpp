
#include "RGBPanel.h"
#include "RGBPanelInit.h"
// #include "SensorWireHelper.h"

static const lcd_init_cmd_t *_init_cmd = NULL;
static spi_device_handle_t _display_spi = NULL;

uint8_t LCD_Backlight = 50;

void Set_Backlight(uint8_t Light)
{
  if (Light > Backlight_MAX)
  {
    printf("Set Backlight parameters in the range of 0 to 100 \r\n");
    return;
  }

  const uint32_t maxDuty = (1U << Resolution) - 1;
  const uint32_t backlight = (static_cast<uint32_t>(Light) * maxDuty) / Backlight_MAX;
  ledcWrite(LCD_Backlight_PIN, backlight); // Arduino-ESP32 3.x: LEDC is addressed by pin
}

void Backlight_Init()
{
  ledcAttach(LCD_Backlight_PIN, Frequency, Resolution);
  Set_Backlight(LCD_Backlight);
}

// Called from the RGB driver's vsync interrupt.
static bool IRAM_ATTR onVsync(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *, void *ctx)
{
  BaseType_t woken = pdFALSE;
  xSemaphoreGiveFromISR((SemaphoreHandle_t)ctx, &woken);
  return woken == pdTRUE;
}

RGBPanel::RGBPanel(/* args */)
    : _brightness(0), _panelDrv(NULL),
      _order(RGB_ORDER_RGB),
      _has_init(false),
      _IO_Init(false)
{
}

RGBPanel::~RGBPanel()
{
  releaseBus();
  if (_panelDrv)
  {
    esp_lcd_panel_del(_panelDrv);
    _panelDrv = NULL;
  }
}

bool RGBPanel::begin(disp_lcd_panel_config_t *disp, RGBPanel_Color_Order order, bool IO_Init)
{
  if (_panelDrv)
  {
    return true;
  }
  _init_cmd = st7701_2_8_inches;

  _order = order;
  _disp = disp;
  _IO_Init = IO_Init;

  if (_IO_Init)
  {
    pinMode(_disp->cs_pin, OUTPUT);
    pinMode(_disp->rst_pin, OUTPUT);
    pinMode(_disp->bl_pin, OUTPUT);
  }

  Panel_Reset();

  spi_bus_config_t bus_config = {
      .mosi_io_num = _disp->sda_pin,
      .miso_io_num = -1,
      .sclk_io_num = _disp->sck_pin,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 64,
  };
  esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
  if (err != ESP_OK)
  {
    Serial.printf("LCD SPI bus init failed: %s\\n", esp_err_to_name(err));
    return false;
  }

  spi_device_interface_config_t device_config = {
      .command_bits = 1,
      .address_bits = 8,
      .mode = SPI_MODE0,
      .clock_speed_hz = 40000000,
      .spics_io_num = -1,
      .queue_size = 1,
  };
  err = spi_bus_add_device(SPI2_HOST, &device_config, &_display_spi);
  if (err != ESP_OK)
  {
    Serial.printf("LCD SPI device init failed: %s\\n", esp_err_to_name(err));
    spi_bus_free(SPI2_HOST);
    return false;
  }

  Panel_CS_EN();

  for (int i = 0; i < ST7701_2_8_INCH_DATA_COMMANDS; i++)
  {
    writeCommand(_init_cmd[i].cmd);
    Serial.printf("Writing command: 0x%02X\n", _init_cmd[i].cmd);
    if (_init_cmd[i].databytes > 0)
    {
      Serial.printf("Writing %d data bytes\n", _init_cmd[i].databytes);
      writeData(_init_cmd[i].data, _init_cmd[i].databytes);
    }
    if (_init_cmd[i].delay > 0)
    {
      delay(_init_cmd[i].delay);
    }
  }
  Panel_CS_DIS();

  log_i("=================Complete Init Cmd====================");

  const int bus_rbg_order[SOC_LCD_RGB_DATA_WIDTH] = {
      // BOARD_TFT_DATA12,    //LSB
      _disp->r1_pin,
      _disp->r2_pin,
      _disp->r3_pin,
      _disp->r4_pin,
      _disp->r5_pin,

      _disp->g1_pin,
      _disp->g2_pin,
      _disp->g3_pin,
      _disp->g4_pin,
      _disp->g5_pin,

      // BOARD_TFT_DATA6,     //LSB
      _disp->b1_pin,
      _disp->b2_pin,
      _disp->b3_pin,
      _disp->b4_pin,
      _disp->b5_pin,
  };

  const int bus_rgb_order[SOC_LCD_RGB_DATA_WIDTH] = {
      _disp->b1_pin, _disp->b2_pin, _disp->b3_pin, _disp->b4_pin, _disp->b5_pin,
      _disp->g0_pin, _disp->g1_pin, _disp->g2_pin, _disp->g3_pin, _disp->g4_pin, _disp->g5_pin,
      _disp->r1_pin, _disp->r2_pin, _disp->r3_pin, _disp->r4_pin, _disp->r5_pin};

  esp_lcd_rgb_panel_config_t panel_config = {};
  panel_config.clk_src = LCD_CLK_SRC_PLL240M;
  panel_config.timings.pclk_hz = _disp->pclk_freq;
  panel_config.timings.h_res = _disp->width;
  panel_config.timings.v_res = _disp->height;
  panel_config.timings.hsync_pulse_width = 8;
  panel_config.timings.hsync_back_porch = 10;
  panel_config.timings.hsync_front_porch = 50;
  panel_config.timings.vsync_pulse_width = 2;
  panel_config.timings.vsync_back_porch = 18;
  panel_config.timings.vsync_front_porch = 8;
  panel_config.timings.flags.pclk_active_neg = 0;
  panel_config.data_width = 16; // RGB565 in parallel mode, thus 16bit in width
  panel_config.bits_per_pixel = 16;
  panel_config.num_fbs = (_disp->num_fbs > 1) ? 2 : 1;
  // The CPU copies stripes of the PSRAM frame buffer into small internal-RAM bounce buffers, so the
  // pixel clock is no longer limited by PSRAM DMA latency. Must divide width*height evenly.
  panel_config.bounce_buffer_size_px = (size_t)_disp->bounce_lines * _disp->width;
  panel_config.dma_burst_size = 64;
  panel_config.hsync_gpio_num = _disp->hsync_pin;
  panel_config.vsync_gpio_num = _disp->vsync_pin;
  panel_config.de_gpio_num = _disp->de_pin;
  panel_config.pclk_gpio_num = _disp->pclk_pin;
  panel_config.disp_gpio_num = GPIO_NUM_NC;
  memcpy(panel_config.data_gpio_nums, bus_rgb_order, sizeof(bus_rgb_order));
  panel_config.flags.fb_in_psram = 1; // allocate frame buffer(s) in PSRAM

  if (_order == RGB_ORDER_BGR)
  {
    // Swap color order
    uint8_t data = 0x00;
    Panel_CS_EN();
    writeCommand(0x36);
    writeData(&data, 1);
    Panel_CS_DIS();

    memcpy(panel_config.data_gpio_nums,
           bus_rbg_order,
           sizeof(panel_config.data_gpio_nums));
  }

  err = esp_lcd_new_rgb_panel(&panel_config, &_panelDrv);
  if (err != ESP_OK && panel_config.num_fbs > 1)
  {
    Serial.printf("RGB panel with %d frame buffers failed (%s), retrying with 1\n", panel_config.num_fbs, esp_err_to_name(err));
    panel_config.num_fbs = 1;
    err = esp_lcd_new_rgb_panel(&panel_config, &_panelDrv);
  }
  if (err != ESP_OK)
  {
    Serial.printf("RGB panel create failed: %s\n", esp_err_to_name(err));
    _panelDrv = NULL;
    return false;
  }
  _numFbs = panel_config.num_fbs;

  _vsync = xSemaphoreCreateBinary();
  esp_lcd_rgb_panel_event_callbacks_t cbs = {};
  cbs.on_vsync = onVsync;
  ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(_panelDrv, &cbs, _vsync));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(_panelDrv));
  ESP_ERROR_CHECK(esp_lcd_panel_init(_panelDrv));

  void *fb0 = nullptr, *fb1 = nullptr;
  if (_numFbs == 2)
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(_panelDrv, 2, &fb0, &fb1));
  else
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(_panelDrv, 1, &fb0));
  _fbs[0] = (uint16_t *)fb0;
  _fbs[1] = (uint16_t *)fb1;
  Serial.printf("RGB panel up: %ux%u, %u frame buffer(s), pclk %u Hz\n", _disp->width, _disp->height, _numFbs, (unsigned)_disp->pclk_freq);

  // setBrightness(255);
  // releaseBus();

  return true;
}

void RGBPanel::setBrightness(uint8_t value)
{
  if (value > 0)
  {
    digitalWrite(_disp->bl_pin, HIGH);
  }
  else
  {
    digitalWrite(_disp->bl_pin, LOW);
  }
  _brightness = value;
}

uint8_t RGBPanel::getBrightness()
{
  return _brightness;
}

uint16_t RGBPanel::width()
{
  return _disp->width;
}

uint16_t RGBPanel::height()
{
  return _disp->height;
}

void RGBPanel::releaseBus()
{
  // if (SPI_handle == nullptr)
  // {
  //   return;
  // }

  // esp_err_t ret = spi_bus_remove_device(SPI_handle);
  // if (ret != ESP_OK)
  // {
  //   log_e("Failed to remove display SPI device: %s", esp_err_to_name(ret));
  //   return;
  // }

  // SPI_handle = nullptr;
  // ret = spi_bus_free(SPI2_HOST);
  // if (ret != ESP_OK)
  // {
  //   log_e("Failed to free display SPI bus: %s", esp_err_to_name(ret));
  // }
}

void RGBPanel::writeCommand(const uint8_t cmd)
{
  spi_transaction_t transaction = {
      .cmd = 0,
      .addr = cmd,
      .length = 0,
      .rxlength = 0,
  };
  ESP_ERROR_CHECK(spi_device_transmit(_display_spi, &transaction));
}

void RGBPanel::writeData(const uint8_t *data, int len)
{
  for (int i = 0; i < len; ++i)
  {
    spi_transaction_t transaction = {
        .cmd = 1,
        .addr = data[i],
        .length = 0,
        .rxlength = 0,
    };
    ESP_ERROR_CHECK(spi_device_transmit(_display_spi, &transaction));
  }
}

void RGBPanel::pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *data)
{
  assert(_panelDrv);
  // esp_lcd takes the end corner (exclusive), not a size; identical for full-screen pushes from (0,0).
  esp_lcd_panel_draw_bitmap(_panelDrv, x, y, x + width, y + height, data);
}

uint16_t *RGBPanel::frameBuffer(uint8_t i)
{
  return (i < _numFbs) ? _fbs[i] : nullptr;
}

void RGBPanel::showFrameBuffer(uint16_t *fb, uint32_t timeout_ms)
{
  assert(_panelDrv);
  xSemaphoreTake(_vsync, 0); // drop a stale vsync
  // Passing one of the driver's own frame buffers makes it switch scan-out to it at the next vsync, no copy.
  esp_lcd_panel_draw_bitmap(_panelDrv, 0, 0, _disp->width, _disp->height, fb);
  xSemaphoreTake(_vsync, pdMS_TO_TICKS(timeout_ms));
}
