#pragma once
// Virtual PCA9685. The Bottango driver (built with USE_ADAFRUIT_PWM_LIBRARY)
// registers "I2C servo" effectors against Adafruit_PWMServoDriver and calls
// writeMicroseconds(channel, us) every loop. This robot has no PCA9685 -- the
// servos are SC/ST bus servos -- so this shim, found by the driver's
// #include <Adafruit_PWMServoDriver.h> instead of the real library, forwards
// each write to BottangoServoBridge, which turns it into an ID/position/time
// command for the servo bus.
//
// Bottango "I2C servo" fields -> SC servo:  channel = servo ID,
// address ignored, pulse width (us) = position (see BottangoServoBridge).

#include <Arduino.h>
#include <Wire.h> // the driver's PCA9685 container calls Wire.setClock(400000) (same rate this robot already uses)
#include "BottangoServoBridge.h"

class Adafruit_PWMServoDriver
{
public:
    explicit Adafruit_PWMServoDriver(uint8_t addr = 0x40) : _addr(addr) {}

    bool begin(uint8_t = 0) { return true; }
    void setPWMFreq(float) {}
    void setOscillatorFrequency(uint32_t) {}

    uint8_t setPWM(uint8_t, uint16_t, uint16_t) { return 0; } // raw ticks unused by Bottango's effectors

    uint8_t writeMicroseconds(uint8_t channel, uint16_t us)
    {
        BottangoServoBridge::onPulse(_addr, channel, us);
        return 0;
    }

private:
    uint8_t _addr;
};
