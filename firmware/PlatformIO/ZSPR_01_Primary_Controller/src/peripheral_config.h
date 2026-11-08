#pragma once

// Compile-time enable/disable for each peripheral subsystem. Set to 0 to
// fully compile a peripheral out (its driver, CLI commands, and protocol
// command handlers all disappear).

#define DEBUG

#define ZSPR_CONTROLLER_VERSION "0.0.1"

#define ENABLE_TOF            0
#define ENABLE_CURRENT_SENSE  1
#define ENABLE_GT911_TOUCH    1 // GT911 capacitive touch (polled via Display_Module)
#define ENABLE_CAP_TOUCH      0 // Capacitive touch 
#define ENABLE_IMU_COMPASS    1
#define ENABLE_SD             0
#define ENABLE_BATTERY        1 // Serial2 link to the battery controller board: protocol defined, not yet validated against real hardware
#define ENABLE_BOTTANGO       1 // Bottango app (USB) driving the SC servos; entered/left with HAL_Robot_Module::set_mode()

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


#if (TEST + DEV + PROD) != 1
#error "Exactly one of TEST, DEV, PROD must be set to 1 in peripheral_config.h"
#endif

/******************************/
/* Task related configuration */
/******************************/

// The robot-control task: runs the robot's procedures (calibration) and, once added, its
// kinematics. Period 20 ms, same core as the servo bus task.
#define ROBOT_TASK_STACK_BYTES 6144
#define ROBOT_TASK_PRIORITY 2
#define ROBOT_TASK_PERIOD_MS 20
#define ROBOT_TASK_CORE 1

// The peripheral task: runs every enabled module's update() once per period.
// Servo, display/touch, Bottango and the CLI run in their own tasks and talk
// to these modules through get/set/command (see Universal_Module).
#define PERIPHERAL_TASK_STACK_BYTES 8192
#define PERIPHERAL_TASK_PRIORITY 2 // above the Arduino loop task (1), below the system tasks
#define PERIPHERAL_TASK_PERIOD_MS 10
#define PERIPHERAL_TASK_CORE 0 // Arduino's loop() runs on core 1

// Task watchdog on the peripheral task: it must finish a full pass over the
// modules (and so feed the watchdog) within this time. A module hung in a bus
// transaction, or any lock that never releases, trips it. The TWDT is shared
// system-wide, so this timeout also applies to the idle-task monitoring.
#define PERIPHERAL_WDT_TIMEOUT_MS 2000

// CLI task: polls the serial line and runs commands. Below the peripheral task
// so a command never delays sensor updates. Also watched by the TWDT.
#define CLI_TASK_STACK_BYTES 8192
#define CLI_TASK_PRIORITY 1
#define CLI_TASK_PERIOD_MS 10
#define CLI_TASK_CORE 0

// Servo bus task: polls feedback from every servo on the half-duplex UART. Bus
// transactions block on the servos' replies (up to the library's IOTimeOut when
// one is silent), so this must not share a task with the sensors. Core 1 is
// otherwise idle (loop() only sleeps).
#define SERVO_TASK_STACK_BYTES 6144
#define SERVO_TASK_PRIORITY 2
#define SERVO_TASK_PERIOD_MS 20
#define SERVO_TASK_CORE 1

// Bottango task: runs BottangoCore's loop while Bottango mode is active, and
// idles otherwise. BottangoCore is not thread-safe, so every call into it is
// made from this task. Active: 1 ms loop (animation timing); idle: slow poll
// for a start request.
#define BOTTANGO_TASK_STACK_BYTES 8192
#define BOTTANGO_TASK_PRIORITY 2
#define BOTTANGO_TASK_ACTIVE_MS 1
#define BOTTANGO_TASK_IDLE_MS 50
#define BOTTANGO_TASK_CORE 1

// The one task that draws. It owns the screen: nothing else may write to the
// frame buffers, so ZGFX needs no locking. Other tasks influence what is shown
// only through module state (read with get()) or by posting to a queue/flag that
// this task polls.
//
// The task itself is generic. What is on screen is the active UI_Base: the
// home screen (the robot face, with the app overlay) or one of the apps declared
// in UI_Apps.h.
//
//   touch  : Display task -> display->touchQueue() -> active->touch()
//   draw   : active->frame() into display->backBuffer(), then present()
//   switch : overlay icon / app Home button -> takeLaunch()/takeHome() here
#define UI_TASK_STACK_BYTES 8192
#define UI_TASK_PRIORITY 1 // below the HAL tasks: the screen must never delay sensor reads
#define UI_TASK_PERIOD_MS 20 // 50 fps cap; a changing frame also waits ~2 vsyncs in present()
#define UI_TASK_CORE 1