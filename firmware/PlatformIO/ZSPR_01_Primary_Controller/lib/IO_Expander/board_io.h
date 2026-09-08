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

inline void board_togglePin(int pin) {
    board_digitalWrite(pin, !board_digitalRead(pin));
}

// --- Interrupts on expander pins -------------------------------------------
//
// The KTS1622 chain has no CPU interrupt line of its own. A board wires one
// module's INT output to a real ESP32 GPIO, and may cascade further
// modules' INT outputs into a pin on an earlier module instead of a second
// native GPIO -- so that module's status only shows up as a bit in the
// earlier module's status register. This file has no compile-time
// knowledge of any particular board's pin numbers for that: call
// board_configureInterruptLine() / board_setInterruptCascadePin() once at
// startup with whatever pins your board actually uses (from your own
// hardware_config.h or similar), before attaching any expander-pin
// interrupt.

// One native GPIO (module 0's INT line) plus up to MAX_MODULES-1 cascaded
// module INT pins is enough to cover any depth of KTS1622 chain.
struct BoardInterruptConfig {
    int nativeIntPin = -1;
    int cascadePins[KTS1622_IO_Expander::MAX_MODULES - 1] = {};
};

// Function-local statics inside an inline function: the pre-C++17-safe way
// to get one shared instance out of a header with no companion .cpp (this
// project doesn't build with -std=gnu++17, so a plain `inline` variable
// here would risk one copy per translation unit).
inline BoardInterruptConfig &_boardInterruptConfig() {
    static BoardInterruptConfig cfg = [] {
        BoardInterruptConfig c;
        for (int &p : c.cascadePins) p = -1;
        return c;
    }();
    return cfg;
}

// The native ESP32 GPIO wired to module 0's KTS1622 INT output. Required
// before any expander-pin interrupt can actually fire.
inline void board_configureInterruptLine(int nativeIntPin) {
    _boardInterruptConfig().nativeIntPin = nativeIntPin;
}

// Declares that `module`'s INT output is wired into `pin` (an expander pin,
// in this file's unified pin numbering, e.g. IO_PIN_0) on an earlier
// module rather than its own native GPIO. `module` is 1..MAX_MODULES-1.
// Call once per cascaded module; modules with a real native GPIO of their
// own don't need this.
inline void board_setInterruptCascadePin(uint8_t module, int pin) {
    if (module == 0 || module > KTS1622_IO_Expander::MAX_MODULES - 1) return;
    _boardInterruptConfig().cascadePins[module - 1] = pin;
}

#define BOARD_MAX_EXPANDER_PIN (KTS1622_IO_Expander::MAX_MODULES * 16)

struct BoardInterruptEntry {
    void (*isr)() = nullptr;
};

inline BoardInterruptEntry *_boardExpanderIsrs() {
    static BoardInterruptEntry table[BOARD_MAX_EXPANDER_PIN];
    return table;
}

inline volatile bool &_boardExpanderIrqPending() {
    static volatile bool pending = false;
    return pending;
}

inline void IRAM_ATTR _boardExpanderIsrTrampoline() {
    _boardExpanderIrqPending() = true;
}

inline KTS1622_InterruptEdge _boardArduinoModeToEdge(int mode) {
    switch (mode) {
        case RISING:  return KTS1622_INT_RISING_EDGE;
        case FALLING: return KTS1622_INT_FALLING_EDGE;
        default:      return KTS1622_INT_ANY_EDGE; // CHANGE, or anything else
    }
}

inline void board_attachInterrupt(int pin, void (*isr)(), int mode) {
    if (!IO_IS_EXPANDER_PIN(pin)) {
        attachInterrupt(digitalPinToInterrupt(pin), isr, mode);
        return;
    }

    uint8_t localPin = (uint8_t)IO_EXPANDER_LOCAL_PIN(pin);
    if (localPin >= BOARD_MAX_EXPANDER_PIN) return;

    _boardExpanderIsrs()[localPin].isr = isr;
    expander.setInterruptEdge(localPin, _boardArduinoModeToEdge(mode));
    expander.setInterruptEnabled(localPin, true);

    BoardInterruptConfig &cfg = _boardInterruptConfig();

    // Arm every cascade pin between module 0 and this pin's module, so the
    // interrupt actually propagates up to a real hardware line.
    uint8_t targetModule = localPin / 16;
    for (uint8_t m = 0; m < targetModule; m++) {
        int cascadePin = cfg.cascadePins[m];
        if (cascadePin < 0) continue;
        uint8_t cascadeLocal = (uint8_t)IO_EXPANDER_LOCAL_PIN(cascadePin);
        expander.setInterruptEdge(cascadeLocal, KTS1622_INT_ANY_EDGE);
        expander.setInterruptEnabled(cascadeLocal, true);
    }

    static bool nativeIsrAttached = false;
    if (!nativeIsrAttached && cfg.nativeIntPin >= 0) {
        attachInterrupt(digitalPinToInterrupt(cfg.nativeIntPin), _boardExpanderIsrTrampoline, FALLING);
        nativeIsrAttached = true;
    }
}

inline void board_detachInterrupt(int pin) {
    if (!IO_IS_EXPANDER_PIN(pin)) {
        detachInterrupt(digitalPinToInterrupt(pin));
        return;
    }

    uint8_t localPin = (uint8_t)IO_EXPANDER_LOCAL_PIN(pin);
    if (localPin >= BOARD_MAX_EXPANDER_PIN) return;

    expander.setInterruptEnabled(localPin, false);
    _boardExpanderIsrs()[localPin].isr = nullptr;
}

// Call every loop() iteration. Drains the flag the native ISR raises: reads
// module 0's status, dispatches/clears any registered pins, then follows
// the configured cascade pin(s) into module 1, 2, ... as far as the
// interrupt actually propagated.
inline void board_serviceInterrupts() {
    if (!_boardExpanderIrqPending()) return;
    _boardExpanderIrqPending() = false;

    BoardInterruptEntry *isrs = _boardExpanderIsrs();
    BoardInterruptConfig &cfg = _boardInterruptConfig();

    uint8_t moduleIndex = 0;
    bool cascaded = true;
    while (cascaded) {
        uint16_t status = expander.moduleInterruptStatus(moduleIndex);
        for (uint8_t bit = 0; bit < 16; bit++) {
            if (status & (1u << bit)) {
                uint8_t globalPin = moduleIndex * 16 + bit;
                expander.clearInterrupt(globalPin);
                if (isrs[globalPin].isr) isrs[globalPin].isr();
            }
        }

        cascaded = false;
        if (moduleIndex < KTS1622_IO_Expander::MAX_MODULES - 1) {
            int cascadePin = cfg.cascadePins[moduleIndex];
            if (cascadePin >= 0) {
                uint8_t cascadeLocal = (uint8_t)IO_EXPANDER_LOCAL_PIN(cascadePin);
                if (status & (1u << cascadeLocal)) {
                    moduleIndex++;
                    cascaded = true;
                }
            }
        }
    }
}
