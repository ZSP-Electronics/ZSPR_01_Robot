#pragma once

#include <cstring>
#include <cstdlib>
#include <math.h>

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SimpleCLI.h"
#include "KTS1622.h"
#include "Buzzer.h"

#include "peripheral_config.h"
#include "hardware_config.h"
#include "board_io.h"
#include "Current_Module.h"
#include "Servo_Module.h"
#include "Display_Module.h"
#include "IMU_Module.h"
#include "TOF_Module.h"

SimpleCLI cli;
Command cmdHelp;

const uint8_t addrs[] = {KTS1622_ADDR_GND, KTS1622_ADDR_VDD}; // up to 4
KTS1622_IO_Expander expander(2, addrs, Wire);
bool expander_ok = false;

#if ENABLE_BUZZER
Buzzer buzzer(BOARD_BUZZER);
#endif

#if ENABLE_CURRENT_SENSE
Current_Module current_module(ENABLE_CURRENT_SENSE);
return_codes_t current_module_ok = ERROR;
#endif

#if ENABLE_SERVO
Servo_Module servo_module(ENABLE_SERVO);
return_codes_t servo_module_ok = ERROR;
#endif

#if ENABLE_DISPLAY
Display_Module display_module(ENABLE_DISPLAY);
#endif

#if ENABLE_IMU_COMPASS
IMU_Module imu_module(ENABLE_IMU_COMPASS);
return_codes_t imu_module_ok = ERROR;
#endif

#if ENABLE_TOF
TOF_Module tof_module(ENABLE_TOF);
return_codes_t tof_module_ok = ERROR;
#endif

void hardware_setup(void)
{
  /* Initialize serial communication */
  Serial.begin(115200); // USB CDC Connection

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  Wire.setClock(400000); // Set I2C frequency to 400kHz

  expander_ok = expander.begin();
  board_pinMode(BOARD_LED, OUTPUT);
  board_pinMode(BOARD_TFT_BL, OUTPUT);
  board_pinMode(BOARD_TFT_RST, OUTPUT);
  board_pinMode(BOARD_TFT_CS, OUTPUT);
  board_pinMode(BOARD_TOUCH_IRQ, INPUT);
  board_pinMode(BOARD_TOUCH_RST, OUTPUT);
  board_pinMode(BOARD_SDMMC_EN, OUTPUT);
  board_pinMode(BOARD_SDMMC_DET, INPUT);
  board_pinMode(BOARD_IO_INT, INPUT);
  // attachInterrupt(digitalPinToInterrupt(BOARD_IO_INT), io_int_isr, FALLING);
  board_pinMode(BOARD_IO_RST, OUTPUT);
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
  board_digitalWrite(BOARD_LED, LOW);
#endif

#if ENABLE_CURRENT_SENSE
  current_module_ok = current_module.setup();
#endif

#if ENABLE_SERVO
  servo_module_ok = servo_module.setup();
#endif

#if ENABLE_IMU_COMPASS
  imu_module_ok = imu_module.setup();
#endif

#if ENABLE_TOF
  tof_module_ok = tof_module.setup();
#endif
}

void hardware_update(void)
{
#if ENABLE_CURRENT_SENSE
  current_module.update();
#endif

#if ENABLE_SERVO
  servo_module.update();
#endif

#if ENABLE_IMU_COMPASS
  imu_module.update();
#endif

#if ENABLE_TOF
  tof_module.update();
#endif
}


/* COMMAND LINE INTERFACE */
void errorCallback(cmd_error* errorPtr) {
  CommandError e(errorPtr);

  Serial.println("ERROR: " + e.toString());

  if (e.hasCommand()) {
    Serial.println("Did you mean? " + e.getCommand().toString());
  } else {
    Serial.println(cli.toString());
  }
}

void helpCallback(cmd* cmdPtr) {
  Command cmd(cmdPtr);
  Serial.println("\r\nCommands:");
  Serial.println(cli.toString());
}

/**************************************/
/*            CLI SETUP               */
/**************************************/

void cli_setup() {
  hardware_setup();

  cli.setOnError(errorCallback);
  cmdHelp = cli.addCommand("help", helpCallback);
}