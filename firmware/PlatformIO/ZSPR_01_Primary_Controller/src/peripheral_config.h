#pragma once

// Compile-time enable/disable for each peripheral subsystem. Set to 0 to
// fully compile a peripheral out (its driver, CLI commands, and protocol
// command handlers all disappear).

#define DEBUG

#define ENABLE_SERVO          0
#define ENABLE_TOF            1
#define ENABLE_CURRENT_SENSE  1
#define ENABLE_TOUCH          0 // GT911 capacitive touch (polled via Display_Module)
#define ENABLE_IMU_COMPASS    1
#define ENABLE_SD             0
#define ENABLE_DISPLAY        1 // RGB parallel panel: frame buffer needs PSRAM (RGBPanel.h #errors without it); disable on non-PSRAM S3 modules
#define ENABLE_BUZZER         1
#define ENABLE_BATTERY        0 // Serial2 link to the battery controller board: protocol defined, not yet validated against real hardware
#define ENABLE_BOTTANGO       0 // Bottango app (USB) + offline animations driving the SC servos; requires ENABLE_SERVO

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
