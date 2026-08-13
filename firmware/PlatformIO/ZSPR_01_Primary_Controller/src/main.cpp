#include <Arduino.h>
#include <SPI.h>
#include "KTS1622.h"
#include "hardware_config.h"
#include "board_io.h"
#include "RGBPanel.h"
#include "RGBRoboFace.h"

#include <TFT_eSPI.h>

// KTS1622_IO_Expander global pin numbering: module 0 (GND) = expander pins
// 0..15, module 1 (VDD) = expander pins 16..31, matching IO_PIN_0..31 in
// io_defines.h. boardPinMode/boardDigitalWrite/boardDigitalRead (board_io.h)
// dispatch here automatically for any IO_PIN_*/BOARD_* constant.
const uint8_t addrs[] = { KTS1622_ADDR_GND, KTS1622_ADDR_VDD }; // up to 4
KTS1622_IO_Expander expander(2, addrs, Wire);
RGBPanel panel;
RGBroboFace face;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

#define WIDTH panel.width()
#define HEIGHT panel.height()
uint32_t _last_refresh = 0;
uint16_t _target_frame_rate = 30; //in ms will result in ~33FPS

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
  .b5_pin = BOARD_TFT_DATA17
};

void setup() {
  /* Initialize serial communication */
  Serial.begin(115200);
  // Serial1.begin(1000000, SERIAL_8N1, BOARD_MOTOR_RX, BOARD_MOTOR_TX);
  Serial1.begin(1000000);
  Serial2.begin(115200, SERIAL_8N1, BOARD_SERIAL_RX, BOARD_SERIAL_TX);

  SPI.begin(BOARD_SCLK, BOARD_MISO, BOARD_MOSI);

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  Wire.setClock(400000); // Set I2C frequency to 400kHz
  expander.begin();

  board_pinMode(BOARD_LED, OUTPUT);
  board_pinMode(BOARD_TFT_BL, OUTPUT);
  board_pinMode(BOARD_TFT_RST, OUTPUT);
  board_pinMode(BOARD_TFT_CS, OUTPUT);
  board_pinMode(BOARD_TOUCH_IRQ, OUTPUT);
  board_pinMode(BOARD_TOUCH_RST, OUTPUT);
  board_pinMode(BOARD_SDMMC_EN, OUTPUT);
  board_pinMode(BOARD_SDMMC_DET, OUTPUT);
  board_pinMode(BOARD_IO_INT, OUTPUT);
  board_pinMode(BOARD_IO_RST, OUTPUT);
  board_pinMode(BOARD_BUZZER, OUTPUT);
  board_pinMode(BOARD_IO2_INT, OUTPUT);
  board_pinMode(BOARD_IO2_RST, OUTPUT);
  board_pinMode(BOARD_VLx_SPI_N, OUTPUT);
  board_pinMode(BOARD_VLx_NCS, OUTPUT);
  board_pinMode(BOARD_VLx_SYNC, OUTPUT);
  board_pinMode(BOARD_VLx_INT, OUTPUT);
  board_pinMode(BOARD_VLx_LPN, OUTPUT);
  board_pinMode(BOARD_SENSE_RST, OUTPUT);
  board_pinMode(BOARD_SENSE_CHG, OUTPUT);
  board_pinMode(BOARD_SYS_SPI_CS, OUTPUT);
  board_pinMode(BOARD_SYS_PWR_BUT, OUTPUT);
  board_pinMode(BOARD_SYS_PWR_EN, OUTPUT);
  board_pinMode(BOARD_SYS_PWR_SHTDN, OUTPUT);
  board_pinMode(BOARD_IMU_INT1, OUTPUT);
  board_pinMode(BOARD_IMU_INT2, OUTPUT);
  board_pinMode(BOARD_CHG_EN, OUTPUT);
  board_pinMode(BOARD_CHG_DETECT, OUTPUT);
  board_pinMode(BOARD_CURR_ALERT1, OUTPUT);
  board_pinMode(BOARD_CURR_ALERT2, OUTPUT);
  board_pinMode(BOARD_REG_EN, OUTPUT);

  /* Initialize Modules */
  bool rslt = panel.begin(&panel_config, RGB_ORDER_RGB);
  if (!rslt) {
    while (1) {
      Serial.println("Error, failed to initialize Display");
      delay(1000);
    }
  }

  spr.createSprite(WIDTH, HEIGHT);
  spr.setColorDepth(16);
  spr.setSwapBytes(0);

  face.begin(&spr);
  //face.setEyeHeight(100, 150);
  //face.setEyeWidth(150, 75);
  //face.setEyeBorderradius(25);
  //face.setEyeSpacebetween(-10);
  //face.setAutoblinker(true);
  //face.setCuriosity(true);
  //face.setCuriosityOffsets(0, -20);
  //face.setPositionDistance(25);

  face.addMouth(true);
  //face.setCrosshair(true);
  face.setEyeExpression(Normal);

  board_digitalWrite(BOARD_LED, HIGH);
}

void loop() {
  if(millis() - _last_refresh >= _target_frame_rate) {
    _last_refresh = millis();
    face.update();  // update eyes drawings
    panel.pushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
  }
}
