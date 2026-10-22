
#include "RGBPanel.h"
#include "RGBPanelInit.h"
#include <SPI.h>
// #include "SensorWireHelper.h"

// #if ESP_ARDUINO_VERSION <  ESP_ARDUINO_VERSION_VAL(3,0,0)
#include <esp_adc_cal.h>
// #endif

// #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 17)
// #define WAKEUP_LEVEL_SETTING ESP_EXT1_WAKEUP_ANY_LOW
// #define ADC_ATTEN_DB ADC_ATTEN_DB_12
// #else
// #define WAKEUP_LEVEL_SETTING ESP_EXT1_WAKEUP_ALL_LOW
// #define ADC_ATTEN_DB ADC_ATTEN_DB_11
// #endif

static const lcd_init_cmd_t *_init_cmd = NULL;

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
    board_pinMode(_disp->cs_pin, OUTPUT);
    board_pinMode(_disp->rst_pin, OUTPUT);
    board_pinMode(_disp->bl_pin, OUTPUT);
  }

  Panel_Reset();

  spi_bus_config_t buscfg = {
      .mosi_io_num = _disp->sda_pin,
      .miso_io_num = -1,
      .sclk_io_num = _disp->sck_pin,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 64, // ESP32 S3 max size is 64Kbytes
  };
  esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  assert(ret == ESP_OK);

  spi_device_interface_config_t devcfg = {
      .command_bits = 1,
      .address_bits = 8,
      .mode = SPI_MODE0,
      .clock_speed_hz = 40000000,
      .spics_io_num = -1,
      .queue_size = 1, // Not using queues
  };
  ret = spi_bus_add_device(SPI2_HOST, &devcfg, &SPI_handle);
  assert(ret == ESP_OK);

  Panel_CS_EN();

  for (int i = 0; i < ST7701_2_8_INCH_DATA_COMMANDS; i++)
  {
    writeCommand(_init_cmd[i].cmd);
    writeData(_init_cmd[i].data, _init_cmd[i].databytes);
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

  esp_lcd_rgb_panel_config_t panel_config = {
      .clk_src = LCD_CLK_SRC_PLL160M, // LCD_CLK_SRC_PLL160M   LCD_CLK_SRC_PLL240M   LCD_CLK_SRC_XTAL   LCD_CLK_SRC_DEFAULT
      .timings = {
          .pclk_hz = _disp->pclk_freq,
          .h_res = _disp->width,
          .v_res = _disp->height,
          // The following parameters should refer to LCD spec
          .hsync_pulse_width = 1,
          .hsync_back_porch = 30,
          .hsync_front_porch = 50,
          .vsync_pulse_width = 1,
          .vsync_back_porch = 30,
          .vsync_front_porch = 20,
          .flags = {
              .pclk_active_neg = 1,
          },
      },
      .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
      .psram_trans_align = 64,
      .hsync_gpio_num = _disp->hsync_pin,
      .vsync_gpio_num = _disp->vsync_pin,
      .de_gpio_num = _disp->de_pin,
      .pclk_gpio_num = _disp->pclk_pin,

      .data_gpio_nums = {
                         _disp->r1_pin, _disp->r2_pin, _disp->r3_pin, _disp->r4_pin, _disp->r5_pin,
                         _disp->g0_pin, _disp->g1_pin, _disp->g2_pin, _disp->g3_pin, _disp->g4_pin, _disp->g5_pin,
                         _disp->b1_pin, _disp->b2_pin, _disp->b3_pin, _disp->b4_pin, _disp->b5_pin
                        },
      .disp_gpio_num = GPIO_NUM_NC,
      .on_frame_trans_done = NULL,
      .user_ctx = NULL,
      .flags = {
          .fb_in_psram = 1, // allocate frame buffer in PSRAM
      },
  };

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

  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &_panelDrv));
  // esp_lcd_rgb_panel_event_callbacks_t cbs = {
  //   .on_vsync = example_on_vsync_event,
  // };
  // esp_lcd_rgb_panel_register_event_callbacks(_panelDrv, &cbs, &disp_drv);
  // esp_lcd_panel_reset(_panelDrv);
  ESP_ERROR_CHECK(esp_lcd_panel_init(_panelDrv));

  setBrightness(255);
  releaseBus();

  return true;
}

void RGBPanel::setBrightness(uint8_t value)
{
  if (value > 0)
  {
    board_digitalWrite(_disp->bl_pin, HIGH);
  }
  else
  {
    board_digitalWrite(_disp->bl_pin, LOW);
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
  if (SPI_handle == nullptr)
  {
    return;
  }

  esp_err_t ret = spi_bus_remove_device(SPI_handle);
  if (ret != ESP_OK)
  {
    log_e("Failed to remove display SPI device: %s", esp_err_to_name(ret));
    return;
  }

  SPI_handle = nullptr;
  ret = spi_bus_free(SPI2_HOST);
  if (ret != ESP_OK)
  {
    log_e("Failed to free display SPI bus: %s", esp_err_to_name(ret));
  }
}

void RGBPanel::writeCommand(const uint8_t cmd)
{
  spi_transaction_t spi_tran = {
      .cmd = 0,
      .addr = cmd,
      .length = 0,
      .rxlength = 0,
  };
  spi_device_transmit(SPI_handle, &spi_tran);
}

void RGBPanel::writeData(const uint8_t *data, int len)
{
  for (int i = 0; i < len; i++)
  {
    spi_transaction_t spi_tran = {
        .cmd = 1,
        .addr = data[i],
        .length = 0,
        .rxlength = 0,
    };
    spi_device_transmit(SPI_handle, &spi_tran);
  }
}

void RGBPanel::pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *data)
{
  assert(_panelDrv);
  esp_lcd_panel_draw_bitmap(_panelDrv, x, y, width, height, data);
}
