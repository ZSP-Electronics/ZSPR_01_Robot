#pragma once

#include <Arduino.h>

class Servo_Module; // SC09 bus servo driver (src/Servo_Module.h), kept out of this header

// Bridges Bottango pin servos onto SC09 bus servos.
//
//   Bottango pin  == servo bus ID
//   PWM range     == 0-1023  (servo position), or e.g. 500-1523 with begin(500)
//   Angle range   == 0-300
//
// BottangoArduinoDriver is untouched: ZSPR_Servo_Shim.h replaces its Servo class, and that
// class forwards every position write to the singleton below.
class ZSPR_BottangoDriver
{
public:
    static constexpr int MIN_POSITION = 0;
    static constexpr int MAX_POSITION = 1023;
    Servo_Module *_bus = nullptr;

    // Single shared instance (the shim's Servo objects have no other way to reach the bus).
    static ZSPR_BottangoDriver &instance();

    ZSPR_BottangoDriver(const ZSPR_BottangoDriver &) = delete;
    ZSPR_BottangoDriver &operator=(const ZSPR_BottangoDriver &) = delete;

    // Starts the servo bus and scans for servos. Safe to call more than once.
    // Call before BottangoCore::bottangoSetup().
    //
    // signalOffset: use it if Bottango won't accept a PWM range starting at 0. The offset is
    // subtracted from every Bottango signal to get the servo position, so with begin(500) a
    // Bottango PWM range of 500-1523 maps to servo positions 0-1023. Default 0 = no offset.
    void begin(int signalOffset = 0);

    // Writes a Bottango signal to the servo with the given bus ID. The signal minus the offset is
    // clamped to MIN_POSITION..MAX_POSITION. Does nothing until begin() has been called.
    void servoWrite(uint8_t id, int signal);

    int signalOffset() const { return _signalOffset; }

    bool isStarted() const { return _bus != nullptr; }

private:
    ZSPR_BottangoDriver() = default;
    int _signalOffset = 0;
};
