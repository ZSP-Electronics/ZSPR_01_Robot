#pragma once

// Force-included into every C++ translation unit by scripts/zspr_bottango.py.
//
// Bottango's PinServoEffector drives a "Servo" from ESP32Servo.h. Defining ESP32Servo's include
// guard here makes PinServoEffector.h skip that header and use this Servo instead, which sends
// positions to the SC09 bus servo whose ID equals the Bottango pin number. BottangoArduinoDriver
// itself is left unmodified.
//
// Bottango setup: pin = servo bus ID, PWM range 0-1023, angle range 0-300.

#define ESP32_Servo_h

#include <Arduino.h>
#include "ZSPR_Bottango_Driver.h"

class Servo
{
public:
    void setTimerWidth(int) {}

    int attach(int pin)
    {
        _id = (uint8_t)pin;
        _attached = true;
        return pin;
    }

    void detach() { _attached = false; }

    void release() {}

    void writeMicroseconds(int value)
    {
        _value = value;
        if (_attached)
            ZSPR_BottangoDriver::instance().servoWrite(_id, value);
    }

    int readMicroseconds() { return _value; }

private:
    uint8_t _id = 0;
    int _value = 0;
    bool _attached = false;
};
