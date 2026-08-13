#pragma once

#include <Arduino.h>
#include <Wire.h>
// #include "io_defines.h"

// Tag bit marking a pin number as belonging to the KTS1622 IO expander chain
// (see board_io.h) rather than a native ESP32 GPIO. Native ESP32 GPIOs never
// reach this value, so the two pin spaces share a uint8_t without collision.
// IO_PIN_n therefore always means "expander pin n" (global index across the
// chained modules, see KTS1622_IO_Expander), never native GPIO n.
#define IO_EXPANDER_FLAG 0x0080U
#define IO_EXPANDER_PIN(n) (int)(IO_EXPANDER_FLAG | (n))
#define IO_EXPANDER_LOCAL_PIN(pin) ((pin) & ~IO_EXPANDER_FLAG)
#define IO_IS_EXPANDER_PIN(pin) (((pin) & IO_EXPANDER_FLAG) != 0)

#define IO_PIN_0  IO_EXPANDER_PIN(0)
#define IO_PIN_1  IO_EXPANDER_PIN(1)
#define IO_PIN_2  IO_EXPANDER_PIN(2)
#define IO_PIN_3  IO_EXPANDER_PIN(3)
#define IO_PIN_4  IO_EXPANDER_PIN(4)
#define IO_PIN_5  IO_EXPANDER_PIN(5)
#define IO_PIN_6  IO_EXPANDER_PIN(6)
#define IO_PIN_7  IO_EXPANDER_PIN(7)
#define IO_PIN_8  IO_EXPANDER_PIN(8)
#define IO_PIN_9  IO_EXPANDER_PIN(9)
#define IO_PIN_10 IO_EXPANDER_PIN(10)
#define IO_PIN_11 IO_EXPANDER_PIN(11)
#define IO_PIN_12 IO_EXPANDER_PIN(12)
#define IO_PIN_13 IO_EXPANDER_PIN(13)
#define IO_PIN_14 IO_EXPANDER_PIN(14)
#define IO_PIN_15 IO_EXPANDER_PIN(15)
#define IO_PIN_16 IO_EXPANDER_PIN(16)
#define IO_PIN_17 IO_EXPANDER_PIN(17)
#define IO_PIN_18 IO_EXPANDER_PIN(18)
#define IO_PIN_19 IO_EXPANDER_PIN(19)
#define IO_PIN_20 IO_EXPANDER_PIN(20)
#define IO_PIN_21 IO_EXPANDER_PIN(21)
#define IO_PIN_22 IO_EXPANDER_PIN(22)
#define IO_PIN_23 IO_EXPANDER_PIN(23)
#define IO_PIN_24 IO_EXPANDER_PIN(24)
#define IO_PIN_25 IO_EXPANDER_PIN(25)
#define IO_PIN_26 IO_EXPANDER_PIN(26)
#define IO_PIN_27 IO_EXPANDER_PIN(27)
#define IO_PIN_28 IO_EXPANDER_PIN(28)
#define IO_PIN_29 IO_EXPANDER_PIN(29)
#define IO_PIN_30 IO_EXPANDER_PIN(30)
#define IO_PIN_31 IO_EXPANDER_PIN(31)

// I2C device address, selected by the ADDR pin's connection (KTS1622 datasheet Table 1).
// Up to 4 KTS1622s can share a bus, one per ADDR strapping option.
enum KTS1622_Address : uint8_t {
    KTS1622_ADDR_GND = 0x20, // ADDR tied to VSS
    KTS1622_ADDR_VDD = 0x21, // ADDR tied to VDD_I2C
    KTS1622_ADDR_SCL = 0x22, // ADDR tied to SCL
    KTS1622_ADDR_SDA = 0x23, // ADDR tied to SDA
};

// Pin numbering used by this driver: P0_0..P0_7 = 0..7, P1_0..P1_7 = 8..15.
static constexpr uint8_t KTS1622_P0_0 = 0;
static constexpr uint8_t KTS1622_P0_1 = 1;
static constexpr uint8_t KTS1622_P0_2 = 2;
static constexpr uint8_t KTS1622_P0_3 = 3;
static constexpr uint8_t KTS1622_P0_4 = 4;
static constexpr uint8_t KTS1622_P0_5 = 5;
static constexpr uint8_t KTS1622_P0_6 = 6;
static constexpr uint8_t KTS1622_P0_7 = 7;
static constexpr uint8_t KTS1622_P1_0 = 8;
static constexpr uint8_t KTS1622_P1_1 = 9;
static constexpr uint8_t KTS1622_P1_2 = 10;
static constexpr uint8_t KTS1622_P1_3 = 11;
static constexpr uint8_t KTS1622_P1_4 = 12;
static constexpr uint8_t KTS1622_P1_5 = 13;
static constexpr uint8_t KTS1622_P1_6 = 14;
static constexpr uint8_t KTS1622_P1_7 = 15;

enum KTS1622_DriveStrength : uint8_t {
    KTS1622_DRIVE_25_PERCENT  = 0b00,
    KTS1622_DRIVE_50_PERCENT  = 0b01,
    KTS1622_DRIVE_75_PERCENT  = 0b10,
    KTS1622_DRIVE_100_PERCENT = 0b11,
};

enum KTS1622_InterruptEdge : uint8_t {
    KTS1622_INT_LEVEL        = 0b00, // level change since last read of input port register (default)
    KTS1622_INT_RISING_EDGE  = 0b01,
    KTS1622_INT_FALLING_EDGE = 0b10,
    KTS1622_INT_ANY_EDGE     = 0b11,
};

// Driver for the Kinetic Technologies KTS1622 low-voltage 16-bit I2C I/O expander.
// The caller is responsible for calling wirePort.begin(...) before begin().
class KTS1622 {
public:
    explicit KTS1622(uint8_t i2cAddress = KTS1622_ADDR_GND, TwoWire &wirePort = Wire);

    // Probes the device over I2C. Returns true if it acknowledged.
    bool begin();

    // --- Arduino-style single pin I/O, pin = 0..15 ---
    // mode accepts the standard Arduino constants: INPUT, OUTPUT, INPUT_PULLUP,
    // INPUT_PULLDOWN, OUTPUT_OPEN_DRAIN.
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t value);
    int digitalRead(uint8_t pin);

    // --- Whole-port access (port = 0 or 1, 8 bits) ---
    void portMode(uint8_t port, uint8_t inputMask); // bit = 1 -> input, 0 -> output
    void writePort(uint8_t port, uint8_t value);
    uint8_t readPort(uint8_t port);

    // --- Whole-device access (port 1 in the high byte) ---
    void writeAllPorts(uint16_t value);
    uint16_t readAllPorts();

    // Reads the input port registers without clearing latched interrupts.
    uint8_t readPortStatus(uint8_t port);
    uint16_t readAllPortsStatus();

    // --- Input configuration ---
    void setPolarityInversion(uint8_t pin, bool inverted);
    void setInputLatch(uint8_t pin, bool latched);

    // --- Pull resistors ---
    void setPullUpDown(uint8_t pin, bool pullUp); // enables the resistor and selects up/down
    void disablePull(uint8_t pin);

    // --- Output configuration ---
    void setOutputDriveStrength(uint8_t pin, KTS1622_DriveStrength strength);
    void setPortOpenDrain(uint8_t port, bool openDrain); // port-wide push-pull/open-drain (reg 4Fh)
    void setPinOpenDrain(uint8_t pin, bool openDrain);   // per-pin override of the port setting

    // --- Interrupts ---
    void setInterruptEnabled(uint8_t pin, bool enabled);
    void setInterruptEdge(uint8_t pin, KTS1622_InterruptEdge edge);
    uint16_t getInterruptStatus();
    void clearInterrupt(uint8_t pin);
    void clearAllInterrupts();

    // --- Switch debounce (P0_0 must supply the debounce oscillator clock) ---
    void setSwitchDebounceEnabled(uint8_t pin, bool enabled);
    void setDebounceCount(uint8_t count);

    // I2C general-call software reset. This resets every KTS1622 on the shared bus,
    // not just this instance, per the datasheet's software reset procedure.
    static void resetAllDevicesOnBus(TwoWire &wirePort = Wire);

    // --- Low-level register access ---
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);
    void writeRegisterBit(uint8_t reg, uint8_t bit, bool value);

private:
    TwoWire *_wire;
    uint8_t _address;

    static void splitPin(uint8_t pin, uint8_t &port, uint8_t &bit);
};

// ---------------------------------------------------------------------------
// KTS1622Chain — helper superclass to manage up to 4 daisy-chained KTS1622
// modules using a global pin namespace: 0..15 -> module 0, 16..31 -> module 1,
// etc. This keeps the chain helper in the same file as the single-chip driver.
// ---------------------------------------------------------------------------

class KTS1622_IO_Expander {
public:
    static constexpr uint8_t MAX_MODULES = 4;

    // `addresses` must point to at least `numModules` entries.
    KTS1622_IO_Expander(uint8_t numModules, const uint8_t addresses[], TwoWire &wirePort = Wire);
    ~KTS1622_IO_Expander();

    // Probes each device on the bus. Returns true if all modules acknowledge.
    bool begin();

    // Global pin APIs: pin = 0..(numModules*16-1)
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t value);
    int digitalRead(uint8_t pin);

    uint8_t modules() const { return _numModules; }

private:
    KTS1622* _modules[MAX_MODULES];
    uint8_t _numModules;
    TwoWire* _wire;

    bool mapPin(uint8_t pin, uint8_t &moduleIndex, uint8_t &localPin) const;
};
