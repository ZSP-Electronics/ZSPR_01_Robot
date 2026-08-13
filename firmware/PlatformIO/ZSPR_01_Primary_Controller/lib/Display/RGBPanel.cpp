
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

RGBPanel::~RGBPanel() {
  if (_panelDrv) {
    esp_lcd_panel_del(_panelDrv);
    _panelDrv = NULL;
  }
}

bool RGBPanel::begin(disp_lcd_panel_config_t *disp, RGBPanel_Color_Order order, bool IO_Init) {
  if (_panelDrv) {
    return true;
  }

  _order = order;
  _disp = disp;
  _IO_Init = IO_Init;
  initDevice();

  return true;
}

void RGBPanel::initDevice() {
  if(_IO_Init) {
    board_pinMode(_disp->cs_pin, OUTPUT);
    board_pinMode(_disp->rst_pin, OUTPUT);
    board_pinMode(_disp->bl_pin, OUTPUT);
  }

  board_digitalWrite(_disp->bl_pin, HIGH);
  board_digitalWrite(_disp->cs_pin, HIGH);

  _init_cmd = st7701_2_8_inches;

  initBUS();
}


void RGBPanel::setBrightness(uint8_t value) {
  if(value > 0){
    board_digitalWrite(_disp->bl_pin, HIGH);
  } else {
    board_digitalWrite(_disp->bl_pin, LOW);
  }
  _brightness = value;
}

uint8_t RGBPanel::getBrightness() {
  return _brightness;
}


uint16_t RGBPanel::width() {
  return _disp->width;
}

uint16_t RGBPanel::height() {
  return _disp->height;
}

void RGBPanel::initBUS() {
  log_i("=================initBus====================");
  assert(_init_cmd);

  if (_panelDrv) {
    return;
  }

  board_digitalWrite(_disp->rst_pin, LOW);
  delay(20);
  board_digitalWrite(_disp->rst_pin, HIGH);
  delay(10);

  Wire.setClock(1000000UL);
  // uint32_t start = millis();

#if defined(T_RGB_HARDWARE)
  extension.beginSPI(mosi, -1, sclk, cs);
#endif

  board_digitalWrite(_disp->cs_pin, LOW);

  int i = 0;
  while (_init_cmd[i].databytes != 0xff) {
    writeCommand(_init_cmd[i].cmd);
    writeData(_init_cmd[i].data, _init_cmd[i].databytes & 0x1F);
    if (_init_cmd[i].databytes & 0x80) {
      delay(100);
    }
    i++;
  }

  board_digitalWrite(_disp->cs_pin, HIGH);

  log_i("=================Complete Init Cmd====================");

  // uint32_t end = millis();

  // Serial.printf("Initialization took %u milliseconds\n", end - start);

  // Uses 400k I2C speed : Initialization took about 6229 milliseconds
  // Uses 1M   I2C speed : Initialization took about 1833 milliseconds

  // Reduce to standard speed, touch does not support access greater than 400KHZ speed
  Wire.setClock(400000UL);


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
    .clk_src = LCD_CLK_SRC_PLL160M,
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
    .data_width = 16,  // RGB565 in parallel mode, thus 16bit in width
    .psram_trans_align = 64,
    .hsync_gpio_num = _disp->hsync_pin,
    .vsync_gpio_num = _disp->vsync_pin,
    .de_gpio_num = _disp->de_pin,
    .pclk_gpio_num = _disp->pclk_pin,

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    .disp_gpio_num = GPIO_NUM_NC,
    .data_gpio_nums = {
      // BOARD_TFT_DATA0,
      BOARD_TFT_DATA13,
      BOARD_TFT_DATA14,
      BOARD_TFT_DATA15,
      BOARD_TFT_DATA16,
      BOARD_TFT_DATA17,

      BOARD_TFT_DATA6,
      BOARD_TFT_DATA7,
      BOARD_TFT_DATA8,
      BOARD_TFT_DATA9,
      BOARD_TFT_DATA10,
      BOARD_TFT_DATA11,
      // BOARD_TFT_DATA12,

      BOARD_TFT_DATA1,
      BOARD_TFT_DATA2,
      BOARD_TFT_DATA3,
      BOARD_TFT_DATA4,
      BOARD_TFT_DATA5,
    },
#else
    .data_gpio_nums = {
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
    _disp->b5_pin
    },
    .disp_gpio_num = GPIO_NUM_NC,
    .on_frame_trans_done = NULL,
    .user_ctx = NULL,
#endif
    .flags = {
      .fb_in_psram = 1,  // allocate frame buffer in PSRAM
    },
  };

  if (_order == RGB_ORDER_BGR) {

    // Swap color order

    uint8_t data = 0x00;
    board_digitalWrite(_disp->cs_pin, LOW);
    writeCommand(0x36);
    writeData(&data, 1);
    board_digitalWrite(_disp->cs_pin, HIGH);

    memcpy(panel_config.data_gpio_nums,
           bus_rbg_order,
           sizeof(panel_config.data_gpio_nums));
  }

  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &_panelDrv));
  ESP_ERROR_CHECK(esp_lcd_panel_init(_panelDrv));
}


void RGBPanel::writeCommand(const uint8_t cmd) {
  uint16_t data = cmd;
  //send over hardware SPI, declated in main
  SPI.transfer(data);
}

void RGBPanel::writeData(const uint8_t *data, int len) {
  uint32_t i = 0;
  if (len > 0) {
    do {
      // The ninth bit of data, 1, represents data, 0 represents command
      uint16_t pdat = (*(data + i)) | 1 << 8;
      SPI.transfer(pdat);
      i++;
    } while (len--);
  }
}

void RGBPanel::pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t hight, uint16_t *data) {
  assert(_panelDrv);
  esp_lcd_panel_draw_bitmap(_panelDrv, x, y, width, hight, data);
}

