#pragma once

// Compile-time enable/disable for each peripheral subsystem. Set to 0 to
// fully compile a peripheral out (its driver, CLI commands, and protocol
// command handlers all disappear).

#define DEBUG

#define ZSPR_CONTROLLER_VERSION "0.0.1"

#define ENABLE_TOF            1
#define ENABLE_CURRENT_SENSE  1
#define ENABLE_GT911_TOUCH    1 // GT911 capacitive touch (polled via Display_Module)
#define ENABLE_CAP_TOUCH      0 // Capacitive touch 
#define ENABLE_IMU_COMPASS    1
#define ENABLE_SD             0
#define ENABLE_BATTERY        1 // Serial2 link to the battery controller board: protocol defined, not yet validated against real hardware
#define ENABLE_BOTTANGO       0 // Bottango app (USB) + offline animations driving the SC servos; requires ENABLE_SERVO

#define TOTAL_PERIPHERIALS     6 //Peripheral count to be ran in single task. Does not include Bottango, CLI, Servo, Display, or GT911 Touch. These are handled in their own tasks.

// Declare graphics library used
#define USE_ARDUINO_GFX        0
#define USE_TFT_SPI_GFX        0
#define USE_CUSTOM_GFX         1

#if(USE_ARDUINO_GFX + USE_TFT_SPI_GFX + USE_CUSTOM_GFX) != 1
#error "Exactly one of USE_ARDUINO_GFX, USE_TFT_SPI_GFX, USE_CUSTOM_GFX must be set to 1 in peripheral_config.h"
#endif

// Build-time application mode -- selects which loop() body runs (see
// main.cpp). Exactly one of these must be set to 1.
//   TEST:  periodic serial self-check of every enabled peripheral.
//   DEBUG: placeholder, not yet implemented.
//   PROD:  placeholder, not yet implemented.
#define TEST                    1
#define DEV                     0
#define PROD                    0

#if ENABLE_BOTTANGO && !ENABLE_SERVO
#error "ENABLE_BOTTANGO requires ENABLE_SERVO"
#endif

#if (TEST + DEV + PROD) != 1
#error "Exactly one of TEST, DEV, PROD must be set to 1 in peripheral_config.h"
#endif
