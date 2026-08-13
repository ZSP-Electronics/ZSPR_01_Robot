#pragma once

#include <Arduino.h>
#include "KTS1622.h"

// Pin-space dispatcher: routes a pin number to either the KTS1622 IO
// expander chain or the native ESP32 GPIO driver, based on IO_EXPANDER_FLAG
// (see io_defines.h). Use these instead of the bare Arduino calls for any
// pin that might be a BOARD_*/IO_PIN_* constant.
//
// `expander` is defined in main.cpp and must be begin()'d before use.
extern KTS1622_IO_Expander expander;

inline void board_pinMode(int pin, uint8_t mode) {
    if (IO_IS_EXPANDER_PIN(pin)) {
        expander.pinMode(IO_EXPANDER_LOCAL_PIN(pin), mode);
    } else {
        pinMode(pin, mode);
    }
}

inline void board_digitalWrite(int pin, uint8_t value) {
    if (IO_IS_EXPANDER_PIN(pin)) {
        expander.digitalWrite(IO_EXPANDER_LOCAL_PIN(pin), value);
    } else {
        digitalWrite(pin, value);
    }
}

inline int board_digitalRead(int pin) {
    if (IO_IS_EXPANDER_PIN(pin)) {
        return expander.digitalRead(IO_EXPANDER_LOCAL_PIN(pin));
    }
    return digitalRead(pin);
}
