#include <Arduino.h>
#include <SPI.h>
#include "KTS1622.h"
#include "hardware_config.h"
#include "board_io.h"

#include "peripheral_config.h"
#include "Display_Module.h"
#include "Current_Module.h"
#include "Servo_Module.h"

#include <Buzzer.h>

#define CURRENT_MODULE_ID 0
#define SERVO_MODULE_ID 1
// #define DISPLAY_MODULE_ID 2

uint8_t num_modules = 2;
volatile bool io_int_flag = false;
const uint8_t addrs[] = {KTS1622_ADDR_GND, KTS1622_ADDR_VDD}; // up to 4
KTS1622_IO_Expander expander(2, addrs, Wire);

Universal_Module *system_modules[] = {
    new Current_Module(ENABLE_CURRENT_SENSE),
    new Servo_Module(ENABLE_SERVO),
    // new Display_Module(ENABLE_DISPLAY),
};

Buzzer buzzer(BOARD_BUZZER);
Servo_Module *servo_module = static_cast<Servo_Module *>(system_modules[SERVO_MODULE_ID]);

#if TEST
uint32_t servo_update_time = 0;
bool servo_dir = false;
#endif

void io_int_isr()
{
  io_int_flag = true;
}

void setup()
{
  /* Initialize serial communication */
  Serial.begin(115200); // USB CDC Connection

  SPI.begin(BOARD_SCLK, BOARD_MISO, BOARD_MOSI);

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  Wire.setClock(400000); // Set I2C frequency to 400kHz

#if TEST
  while (!Serial)
  {
    delay(10);
  }
  delay(5000);
#endif

  bool expander_ok = expander.begin();

#ifdef DEBUG
  Serial.printf("IO expanders: %u/%u attached%s\n",
                expander.attachedModules(), expander.modules(),
                expander_ok ? "" : " (check failedModulesMask())");
#endif

  board_pinMode(BOARD_LED, OUTPUT);
  board_pinMode(BOARD_TFT_BL, OUTPUT);
  board_pinMode(BOARD_TFT_RST, OUTPUT);
  board_pinMode(BOARD_TFT_CS, OUTPUT);
  board_pinMode(BOARD_TOUCH_IRQ, INPUT);
  board_pinMode(BOARD_TOUCH_RST, OUTPUT);
  board_pinMode(BOARD_SDMMC_EN, OUTPUT);
  board_pinMode(BOARD_SDMMC_DET, INPUT);
  board_pinMode(BOARD_IO_INT, INPUT);
  attachInterrupt(digitalPinToInterrupt(BOARD_IO_INT), io_int_isr, FALLING);
  board_pinMode(BOARD_IO_RST, OUTPUT);
  // board_pinMode(BOARD_BUZZER, OUTPUT);
  board_pinMode(BOARD_IO2_INT, INPUT);
  board_pinMode(BOARD_IO2_RST, INPUT);
  board_pinMode(BOARD_VLx_SPI_N, OUTPUT);
  board_pinMode(BOARD_VLx_NCS, OUTPUT);
  board_pinMode(BOARD_VLx_SYNC, OUTPUT);
  board_pinMode(BOARD_VLx_INT, INPUT);
  board_pinMode(BOARD_VLx_LPN, OUTPUT);
  board_pinMode(BOARD_SENSE_RST, OUTPUT);
  board_pinMode(BOARD_SENSE_CHG, OUTPUT);
  board_pinMode(BOARD_SYS_SPI_CS, OUTPUT);
  board_pinMode(BOARD_SYS_PWR_BUT, INPUT);
  board_pinMode(BOARD_SYS_PWR_EN, OUTPUT);
  board_pinMode(BOARD_SYS_PWR_SHTDN, INPUT);
  board_pinMode(BOARD_IMU_INT1, INPUT);
  board_pinMode(BOARD_IMU_INT2, INPUT);
  board_pinMode(BOARD_CHG_EN, OUTPUT);
  board_pinMode(BOARD_CHG_DETECT, INPUT);
  board_pinMode(BOARD_CURR_ALERT1, INPUT);
  board_pinMode(BOARD_CURR_ALERT2, INPUT);
  board_pinMode(BOARD_REG_EN, OUTPUT);

#if TEST
  board_digitalWrite(BOARD_REG_EN, HIGH);
#endif

  for (uint8_t i = 0; i < num_modules; i++)
  {
    return_codes_t code = system_modules[i]->setup();

#if TEST && defined(DEBUG)
    if (code != SUCCESS)
    {
      switch (code)
      {
      case ERROR:
        Serial.printf("Module %u setup failed with ERROR\n", i);
        break;
      case TIMEOUT:
        Serial.printf("Module %u setup failed with TIMEOUT\n", i);
        break;
      case INVALID_PARAM:
        Serial.printf("Module %u setup failed with INVALID_PARAM\n", i);
        break;
      case NOT_IMPLEMENTED:
        Serial.printf("Module %u setup failed with NOT_IMPLEMENTED\n", i);
        break;
      case NOT_SUPPORTED:
        Serial.printf("Module %u setup failed with NOT_SUPPORTED\n", i);
        break;
      default:
        Serial.printf("Module %u setup failed with unknown code %d\n", i, code);
      }
    }
    else
    {
      Serial.printf("Module %u setup OK\n", i);
    }
#endif
  }

#if TEST
  board_digitalWrite(BOARD_LED, LOW);
  servo_update_time = millis();
#endif
}

void loop()
{
  if (io_int_flag)
  {
    io_int_flag = false;
  }

  // for (uint8_t i = 0; i < num_modules; i++)
  // {
  //   system_modules[i]->update();
  // }

  // servo_module->writePos(1, 4095, 3400, 50);
  // delay(3000);
  // servo_module->writePos(1, 0, 3400, 50);
  // delay(3000);

  // if ((millis() - servo_update_time > 2000) && !servo_dir)
  // {
  //   servo_update_time = millis();
  //   system_modules[SERVO_MODULE_ID]->setData(nullptr, 4, servo_to_4095_argv);
  //   servo_dir = true;
  // }
  // else if ((millis() - servo_update_time > 2000) && servo_dir)
  // {
  //   servo_update_time = millis();
  //   system_modules[SERVO_MODULE_ID]->setData(nullptr, 4, servo_to_0_argv);
  //   servo_dir = false;
  // }

  // buzzer.begin(100);

  // buzzer.sound(NOTE_E7, 80);
  // buzzer.sound(NOTE_E7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_E7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_C7, 80);
  // buzzer.sound(NOTE_E7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_G7, 80);
  // buzzer.sound(0, 240);
  // buzzer.sound(NOTE_G6, 80);
  // buzzer.sound(0, 240);
  // buzzer.sound(NOTE_C7, 80);
  // buzzer.sound(0, 160);
  // buzzer.sound(NOTE_G6, 80);
  // buzzer.sound(0, 160);
  // buzzer.sound(NOTE_E6, 80);
  // buzzer.sound(0, 160);
  // buzzer.sound(NOTE_A6, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_B6, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_AS6, 80);
  // buzzer.sound(NOTE_A6, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_G6, 100);
  // buzzer.sound(NOTE_E7, 100);
  // buzzer.sound(NOTE_G7, 100);
  // buzzer.sound(NOTE_A7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_F7, 80);
  // buzzer.sound(NOTE_G7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_E7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_C7, 80);
  // buzzer.sound(NOTE_D7, 80);
  // buzzer.sound(NOTE_B6, 80);
  // buzzer.sound(0, 160);
  // buzzer.sound(NOTE_C7, 80);
  // buzzer.sound(0, 160);
  // buzzer.sound(NOTE_G6, 80);
  // buzzer.sound(0, 160);
  // buzzer.sound(NOTE_E6, 80);
  // buzzer.sound(0, 160);
  // buzzer.sound(NOTE_A6, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_B6, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_AS6, 80);
  // buzzer.sound(NOTE_A6, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_G6, 100);
  // buzzer.sound(NOTE_E7, 100);
  // buzzer.sound(NOTE_G7, 100);
  // buzzer.sound(NOTE_A7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_F7, 80);
  // buzzer.sound(NOTE_G7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_E7, 80);
  // buzzer.sound(0, 80);
  // buzzer.sound(NOTE_C7, 80);
  // buzzer.sound(NOTE_D7, 80);
  // buzzer.sound(NOTE_B6, 80);
  // buzzer.sound(0, 160);

  // buzzer.end(2000);
}