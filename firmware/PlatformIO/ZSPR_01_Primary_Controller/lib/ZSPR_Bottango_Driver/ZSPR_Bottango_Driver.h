#pragma once

#include <Arduino.h>

class Servo_Module; // SC09 bus servo driver (module/Servo_Module.h), kept out of this header

// Bridges Bottango pin servos onto SC09 bus servos.
//
//   Bottango pin  == servo bus ID
//   PWM range     == 0-1023  (servo position), or e.g. 500-1523 with begin(bus, 500)
//   Angle range   == 0-300
//
// BottangoArduinoDriver is only patched where ZSPR_PATCHES.md says: ZSPR_Servo_Shim.h
// replaces its Servo class, and that class forwards every position write to the
// singleton below.
//
// The servo bus is NOT owned here. It is the HAL's Servo_Module, which has its own
// polling task; this driver borrows it for the time Bottango is active:
//   * begin() pauses the servo task's feedback polling and stops writes waiting for
//     replies, so an animation write never queues behind a poll or a missing servo;
//   * end() puts both back.
// Writes go through Servo_Module::writePosition(), which takes the module and
// servo-bus locks, so they are safe next to CLI/UI calls into the same module.
//
// Everything except hostStopped() runs in the Bottango task (BottangoCore is not
// thread-safe); hostStopped() is a flag read from there too, but kept volatile.
class ZSPR_BottangoDriver
{
public:
    static constexpr int MIN_POSITION = 0;
    static constexpr int MAX_POSITION = 1023;

    // Single shared instance (the shim's Servo objects have no other way to reach the bus).
    static ZSPR_BottangoDriver &instance();

    ZSPR_BottangoDriver(const ZSPR_BottangoDriver &) = delete;
    ZSPR_BottangoDriver &operator=(const ZSPR_BottangoDriver &) = delete;

    // Starts driving `bus` (already set up and scanned by the HAL). Safe to call
    // again while active. Call before BottangoCore::bottangoSetup()/bottangoLoop().
    //
    // signalOffset: use it if Bottango won't accept a PWM range starting at 0. The offset is
    // subtracted from every Bottango signal to get the servo position, so with offset 500 a
    // Bottango PWM range of 500-1523 maps to servo positions 0-1023. Default 0 = no offset.
    void begin(Servo_Module &bus, int signalOffset = 0);

    // Hands the bus back to the servo task. Writes become no-ops again.
    void end();

    // Writes a Bottango signal to the servo with the given bus ID. The signal minus the offset is
    // clamped to MIN_POSITION..MAX_POSITION. Does nothing unless begin() has been called.
    void servoWrite(uint8_t id, int signal);

    int signalOffset() const { return _signalOffset; }

    bool isActive() const { return _bus != nullptr; }

    // Set when the Bottango app sent "stop" (Callbacks::onThisControllerStopped).
    // The module reads it to leave Bottango mode; clearHostStopped() re-arms it.
    void notifyHostStopped() { _hostStopped = true; }
    bool hostStopped() const { return _hostStopped; }
    void clearHostStopped() { _hostStopped = false; }

private:
    ZSPR_BottangoDriver() = default;
    Servo_Module *_bus = nullptr;
    int _signalOffset = 0;
    volatile bool _hostStopped = false;
};
