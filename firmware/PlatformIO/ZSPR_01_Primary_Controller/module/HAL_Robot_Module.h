#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>


#include "KTS1622.h"

#include "peripheral_config.h"
#include "hardware_config.h"
#include "board_io.h"
#include "Universal_Module.h"
#include "Servo_Module.h"
#include "Display_Module.h"
#include "CLI_Module.h"

// #include "Bottango_Module.h"
#include "Current_Module.h"
#include "IMU_Module.h"
#include "TOF_Module.h"
#include "SDCard_Module.h"
#include "Battery_Module.h"
#include "Cap_Touch_Module.h"

// Index of each module in HAL_Robot_Module::_modules. Keep in step with
// TOTAL_PERIPHERIALS in peripheral_config.h.
enum class Peripheral : uint8_t
{
    Current = 0,
    IMU = 1,
    TOF = 2,
    SDCard = 3,
    Battery = 4,
    CapTouch = 5,
};


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

// Display task: runs Display_Module::update(), which polls the GT911 and posts
// TouchEvents to display->touchQueue(). Separate from the UI task so touch
// latency doesn't depend on how long a frame takes to draw.
#define DISPLAY_TASK_STACK_BYTES 4096
#define DISPLAY_TASK_PRIORITY 2
#define DISPLAY_TASK_PERIOD_MS 10
#define DISPLAY_TASK_CORE 1
#define PERIPHERAL_WDT_PANIC 1 // 1 = reset the chip on timeout, 0 = only print the stuck task

class HAL_Robot_Module
{
public:
    // Decalare objects for display, servo, and CLI modules. These are the only modules that run in their own tasks, so they are accessed directly. All other modules are accessed through get/set/command.
    Display_Module *display;
    Servo_Module *servo;
    CLI_Module *cli;

    // The peripheral-task modules, for code outside the HAL (UI, app logic).
    // Talk to them only through get/set/command -- never call update().
    Universal_Module *peripheral(Peripheral p) const { return _modules[(uint8_t)p]; }

    HAL_Robot_Module();

    // One-time: pins, Wire/SPI, every enabled module's setup(), then starts
    // the peripheral task. Call from setup() only.
    void init();

    // Call from loop(). The peripherals run in their own task, so this only
    // yields; there is nothing to service here.
    void update();



private:
    static HAL_Robot_Module *_instance;
    // Owned by pointer, not by value: storing derived modules in a
    // Universal_Module array would slice them down to the base class.
    Universal_Module *_modules[TOTAL_PERIPHERIALS] = {}; // list of modules to add to CLI
    return_codes_t _module_ok[TOTAL_PERIPHERIALS] = {};  // result of each module's setup()

    Universal_Module *module(Peripheral p) const { return _modules[(uint8_t)p]; }

    bool expander_ok = false;
    bool display_ok = false;

    TaskHandle_t _peripheralTask = nullptr;
    TaskHandle_t _cliTask = nullptr;
    TaskHandle_t _servoTask = nullptr;
    TaskHandle_t _displayTask = nullptr;

    // Display task loop: never returns. Started by init().
    void display_update();
    static void s_displayTask(void *arg);

    // Servo task loop: never returns. Started by begin().
    void servo_update();
    static void s_servoTask(void *arg);

    void hardware_setup();
    // Task loop: never returns. Started by begin().
    void hardware_update();
    static void s_peripheralTask(void *arg);

    // CLI task loop: never returns. Started by begin().
    void cli_update();
    static void s_cliTask(void *arg);

#if ENABLE_BATTERY && ENABLE_CURRENT_SENSE
    // CurrentSourceFn is a plain, non-capturing `CurrentReadings(*)()` --
    // needs the same static-thunk treatment as the CLI callbacks.
    Battery_Module::CurrentReadings battery_current_source();
    static Battery_Module::CurrentReadings s_battery_current_source();
#endif
};
