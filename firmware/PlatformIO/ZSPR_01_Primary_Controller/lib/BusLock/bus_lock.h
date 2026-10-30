#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// One recursive mutex per shared bus. Arduino's Wire/SPI are not safe to use
// from two tasks at once: a Wire transaction is several calls
// (beginTransmission/write/endTransmission/requestFrom) and a register
// read-modify-write is two transactions, so every user of a bus must hold its
// lock for the whole operation.
//
// Lock order -- always acquire in this order and release in reverse, or two
// tasks can deadlock:
//     module mutex  ->  SPI  ->  I2C  ->  Servo
// i.e. never take SPI while already holding I2C. (Taking I2C while holding SPI
// is normal: the SD and TOF drivers toggle their chip select through the IO
// expander, which locks I2C.) The servo UART is a leaf: nothing else is taken
// while it is held.
//
// The locks are recursive, so a function that holds a bus can call another
// that takes it again.

enum class Bus : uint8_t
{
    I2C = 0,
    SPI = 1,
    Servo = 2, // half-duplex servo UART shared by SCSCL and SMS_STS (Serial1)
};

// Bit masks a module sets in Universal_Module::_buses to say which buses its
// (third-party) driver uses and therefore must be locked around every call.
constexpr uint8_t BUS_MASK_NONE = 0;
constexpr uint8_t BUS_MASK_I2C = 1 << 0;
constexpr uint8_t BUS_MASK_SPI = 1 << 1;

void bus_take(Bus bus);
void bus_give(Bus bus);

// RAII: holds `bus` for the lifetime of the object.
class BusLock
{
public:
    explicit BusLock(Bus bus) : _bus(bus) { bus_take(_bus); }
    ~BusLock() { bus_give(_bus); }

    BusLock(const BusLock &) = delete;
    BusLock &operator=(const BusLock &) = delete;

private:
    Bus _bus;
};
