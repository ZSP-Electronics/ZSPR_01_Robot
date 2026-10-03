#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "SimpleCLI.h"
#include "HostLink.h"
#include "protocol.h"
#include "protocol_handlers.h"
#include "KTS1622.h"
#include "Buzzer.h"

#include "peripheral_config.h"
#include "hardware_config.h"
#include "board_io.h"
#include "Universal_Module.h"
#include "Current_Module.h"
#include "Servo_Module.h"
#include "Bottango_Module.h"
#include "Display_Module.h"
#include "IMU_Module.h"
#include "TOF_Module.h"
#include "SDCard_Module.h"
#include "Battery_Module.h"

#define ZSPR_CONTROLLER_VERSION "0.0.1"
#define HAL_EVENT_QUEUE_LEN 8

// Fixed-size POD posted through the FreeRTOS event queue -- one slot per
// queued brain command. Mirrors PacketCmd::EVENT_POST's wire payload.
struct RobotEvent
{
    uint32_t id;
    char title[32];
    PacketCmd command;
    uint8_t data[PROTOCOL_MAX_PAYLOAD - 1];
    uint8_t dataLen;
};

// Owns and drives every peripheral on the robot body (servos, display, IMU,
// ToF, SD card, battery link, current sensing), the human CLI, and the
// binary HostLink protocol a "brain controller" (e.g. a Raspberry Pi running
// AI/camera/audio) talks over. The brain can:
//   - PacketCmd::CAPS_QUERY  to discover which peripherals are compiled in
//   - any PacketCmd request, synchronously (request -> immediate response)
//   - PacketCmd::EVENT_POST  to enqueue a PacketCmd to run on a later loop()
//     tick instead, in FIFO order, without blocking on its result
//
// The event queue is a real FreeRTOS xQueueCreate/xQueueSend/xQueueReceive
// FIFO, so post_event() is safe to call from any task or ISR context -- but
// it's drained from the same single loop()-driven task that touches every
// peripheral, same concurrency model as the rest of this class. No second
// task, no mutexes: nothing here runs on more than one task at a time.
class HAL_Robot_Module
{
public:
    HAL_Robot_Module() = default;

    // One-time: pins, Wire/SPI, every enabled module's setup(), CLI command
    // registration, HostLink handler registration, event queue creation.
    void begin();

    // Call every loop(): hostLink.poll(), pop+dispatch at most one queued
    // event, then each enabled peripheral's update().
    void update();

    // Thread/ISR-safe enqueue. Fills outId and returns true on success;
    // false (queue full, or dataLen too large) on failure.
    bool post_event(const char *title, uint8_t titleLen,
                     PacketCmd command, const uint8_t *data, uint8_t dataLen,
                     uint32_t &outId);

private:
    static HAL_Robot_Module *_instance;

    // --- owned globals, migrated from src/system_functions.h ---
    SimpleCLI cli;
    Stream *cliSerial = &Serial;
    Command cmdHelp;
    Command cmdPheriph_Test;
    Command cmdBottango;

    // `expander` itself is a true global (board_io.h: "extern
    // KTS1622_IO_Expander expander;", defined in HAL_Robot_Module.cpp) --
    // every inline board_pinMode/board_digitalWrite/board_digitalRead call
    // anywhere in the codebase (including inside other modules' headers)
    // resolves to that one global by name, so it can't be a class member.
    bool expander_ok = false;

#if ENABLE_BUZZER
    Buzzer buzzer{BOARD_BUZZER};
#endif
#if ENABLE_CURRENT_SENSE
    Current_Module current_module{(bool)ENABLE_CURRENT_SENSE};
    return_codes_t current_module_ok = ERROR;
#endif
#if ENABLE_SERVO
    Servo_Module servo_module{(bool)ENABLE_SERVO};
    return_codes_t servo_module_ok = ERROR;
#endif
#if ENABLE_BOTTANGO
    Bottango_Module bottango_module{(bool)ENABLE_BOTTANGO, servo_module};
#endif
#if ENABLE_DISPLAY
    Display_Module display_module{(bool)ENABLE_DISPLAY};
#endif
#if ENABLE_IMU_COMPASS
    IMU_Module imu_module{(bool)ENABLE_IMU_COMPASS};
    return_codes_t imu_module_ok = ERROR;
#endif
#if ENABLE_TOF
    TOF_Module tof_module{(bool)ENABLE_TOF};
    return_codes_t tof_module_ok = ERROR;
#endif
#if ENABLE_SD
    SDCard_Module sdcard_module{(bool)ENABLE_SD};
    return_codes_t sdcard_module_ok = ERROR;
#endif
#if ENABLE_BATTERY
    Battery_Module battery_module{(bool)ENABLE_BATTERY};
    return_codes_t battery_module_ok = ERROR;
#endif

    QueueHandle_t _eventQueue = nullptr;
    uint32_t _nextEventId = 1;

    void hardware_setup();
    void hardware_update();
    void cli_setup();

    // --- CLI callback bodies (moved from system_functions.h) ---
    void errorCallback(cmd_error *errorPtr);
    void helpCallback(cmd *cmdPtr);
    void pingCallback(cmd *cmdPtr);
    void testPeriphCallback(cmd *cmdPtr);
    void bottangoCallback(cmd *cmdPtr);
    void flushSerial();
    void drainSerialAndStop();

    static void s_errorCallback(cmd_error *errorPtr);
    static void s_helpCallback(cmd *cmdPtr);
    static void s_pingCallback(cmd *cmdPtr);
    static void s_testPeriphCallback(cmd *cmdPtr);
    static void s_bottangoCallback(cmd *cmdPtr);

#if ENABLE_BATTERY && ENABLE_CURRENT_SENSE
    // CurrentSourceFn is a plain, non-capturing `CurrentReadings(*)()` --
    // needs the same static-thunk treatment as the CLI callbacks.
    Battery_Module::CurrentReadings battery_current_source();
    static Battery_Module::CurrentReadings s_battery_current_source();
#endif

    // --- PacketCmd handler bodies ---
    uint8_t capsQueryHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t eventPostHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t servoSetPosHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t servoSetSpeedHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t servoSetTorqueHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t servoReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t tofReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t imuReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t magReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t compassHeadingHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t currentReadSysHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t currentReadMotorHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t touchReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t sdListHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t sdReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t sdWriteHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    uint8_t sdDeleteHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);

    static uint8_t s_pingHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_capsQueryHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_eventPostHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_servoSetPosHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_servoSetSpeedHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_servoSetTorqueHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_servoReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_tofReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_imuReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_magReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_compassHeadingHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_currentReadSysHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_currentReadMotorHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_touchReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_sdListHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_sdReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_sdWriteHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
    static uint8_t s_sdDeleteHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen);
};
