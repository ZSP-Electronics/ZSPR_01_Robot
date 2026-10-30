#pragma once

#include <Arduino.h>
#include <cstring>
#include <cstdlib>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "bus_lock.h"

enum return_codes_t
{
    SUCCESS = 0,
    ERROR = 1,
    TIMEOUT = 2,
    INVALID_PARAM = 3,
    NOT_IMPLEMENTED = 4,
    NOT_SUPPORTED = 5
};

// Small tagged value used to pass data in and out of a module's get/set/command.
// No heap: BYTES points at a caller- or module-owned buffer that must outlive
// the call (read it before the next update()).
struct Value
{
    enum Type : uint8_t
    {
        NONE,
        BOOL,
        INT,
        UINT,
        FLOAT,
        BYTES
    };

    Type type = NONE;
    union
    {
        bool b;
        int32_t i;
        uint32_t u;
        float f;
    };
    const void *data = nullptr; // BYTES only
    uint16_t len = 0;           // BYTES only: length in bytes

    Value() : u(0) {}
    Value(bool v) : type(BOOL), b(v) {}
    Value(int32_t v) : type(INT), i(v) {}
    Value(uint32_t v) : type(UINT), u(v) {}
    Value(float v) : type(FLOAT), f(v) {}
    Value(const void *p, uint16_t n) : type(BYTES), u(0), data(p), len(n) {}

    // NUL-terminated string as BYTES (len includes the NUL). The string must
    // outlive the call.
    static Value str(const char *s) { return Value(s, (uint16_t)(strlen(s) + 1)); }
    // BYTES payload as a C string, or nullptr if this isn't BYTES. Only safe
    // when the sender used str() or otherwise NUL-terminated the buffer.
    const char *asCStr() const { return type == BYTES ? (const char *)data : nullptr; }

    bool is(Type t) const { return type == t; }

    // Numeric conversions: accept any numeric type so callers don't have to
    // know whether the sender used INT, UINT or FLOAT.
    bool asBool() const { return type == BOOL ? b : asInt() != 0; }
    int32_t asInt() const
    {
        switch (type)
        {
        case BOOL:  return b ? 1 : 0;
        case INT:   return i;
        case UINT:  return (int32_t)u;
        case FLOAT: return (int32_t)f;
        default:    return 0;
        }
    }
    uint32_t asUInt() const { return (uint32_t)asInt(); }
    float asFloat() const { return type == FLOAT ? f : (type == UINT ? (float)u : (float)asInt()); }
};

// Base class for every hardware module.
//
// Lifecycle: setup() once, update() every loop.
// Data: get/set address a property by a module-defined ID (each module declares
// its own `enum class XxxProp : uint16_t`); command() is for actions that are
// neither a plain read nor a plain write (calibrate, toggle, self-test, ...).
// Modules override onGet/onSet/onCommand; all default to NOT_SUPPORTED.
//
// Threading: a module is used from more than one task (its update loop, the
// CLI, display, ...), so every entry point is serialized per module.
//   - get/set/command and run_setup/run_update take the module's recursive
//     mutex, plus the Wire/SPI bus locks named in _buses, for their duration.
//     Call run_update(), not update(), from a task loop.
//   - A BYTES Value points into the module's own buffers. Hold a DataLock
//     while you read it, or copy it out first -- the module's task may
//     overwrite it on the next update().
//   - Locks are taken module -> SPI -> I2C (see bus_lock.h); don't invert that.
class Universal_Module
{
public:
    Universal_Module(bool enable = true) : _enabled(enable) { _mutex = xSemaphoreCreateRecursiveMutex(); }
    virtual ~Universal_Module()
    {
        if (_mutex)
            vSemaphoreDelete(_mutex);
    }

    // Not copyable: a copy would slice off the derived module and share (or
    // double-free) the mutex. Store modules by pointer.
    Universal_Module(const Universal_Module &) = delete;
    Universal_Module &operator=(const Universal_Module &) = delete;

    virtual return_codes_t setup() { return SUCCESS; }
    virtual return_codes_t update() { return SUCCESS; }

    // Thread-safe wrappers around setup()/update() for the owning task.
    return_codes_t run_setup()
    {
        IoLock l(*this);
        return setup();
    }
    return_codes_t run_update()
    {
        IoLock l(*this);
        return update();
    }

    // Holds just the module's mutex (not the buses) while alive, so values
    // returned by get() -- BYTES in particular -- stay put. Recursive: get()
    // can still be called while holding one.
    class DataLock
    {
    public:
        // Takes a const module so const accessors can lock too.
        explicit DataLock(const Universal_Module &m) : _m(m) { _m.lock_data(); }
        ~DataLock() { _m.unlock_data(); }
        DataLock(const DataLock &) = delete;
        DataLock &operator=(const DataLock &) = delete;

    private:
        const Universal_Module &_m;
    };

    // Public entry points. Non-virtual so the enum-friendly overloads below are
    // never hidden by a derived class; modules override the on* hooks instead.
    // Accepts a module's `enum class` directly: tof.get(TofProp::Ready, v)
    template <typename E>
    return_codes_t get(E id, Value &out)
    {
        if (!_enabled)
            return NOT_SUPPORTED;
        IoLock l(*this);
        return onGet((uint16_t)id, out);
    }
    template <typename E>
    return_codes_t set(E id, const Value &in)
    {
        if (!_enabled)
            return NOT_SUPPORTED;
        IoLock l(*this);
        return onSet((uint16_t)id, in);
    }
    template <typename E>
    return_codes_t command(E id, const Value &in, Value &out)
    {
        if (!_enabled)
            return NOT_SUPPORTED;
        IoLock l(*this);
        return onCommand((uint16_t)id, in, out);
    }
    template <typename E>
    return_codes_t command(E id)
    {
        Value none, out;
        return command(id, none, out);
    }

    bool enabled() const { return _enabled; }
    bool initialized() const { return _initialized; }

protected:
    virtual return_codes_t onGet(uint16_t id, Value &out)
    {
        (void)id;
        (void)out;
        return NOT_SUPPORTED;
    }
    virtual return_codes_t onSet(uint16_t id, const Value &in)
    {
        (void)id;
        (void)in;
        return NOT_SUPPORTED;
    }
    virtual return_codes_t onCommand(uint16_t id, const Value &in, Value &out)
    {
        (void)id;
        (void)in;
        (void)out;
        return NOT_SUPPORTED;
    }

    bool _enabled = true;
    bool _initialized = false;
    uint8_t _init_attempt = 0;
    uint8_t _max_init_attempts = 3;

    // Buses (BUS_MASK_*) this module's driver touches without locking them
    // itself. Set in the constructor; held around every setup/update/get/
    // set/command. Leave BUS_MASK_NONE for serial-only modules and for ones
    // whose driver already locks (INA232, KTS1622 take the I2C lock per
    // register access).
    uint8_t _buses = BUS_MASK_NONE;

private:
    mutable SemaphoreHandle_t _mutex = nullptr; // mutable: const accessors lock too

    void lock_data() const { xSemaphoreTakeRecursive(_mutex, portMAX_DELAY); }
    void unlock_data() const { xSemaphoreGiveRecursive(_mutex); }

    // Module mutex, then SPI, then I2C -- released in reverse.
    class IoLock
    {
    public:
        explicit IoLock(Universal_Module &m) : _m(m)
        {
            _m.lock_data();
            if (_m._buses & BUS_MASK_SPI)
                bus_take(Bus::SPI);
            if (_m._buses & BUS_MASK_I2C)
                bus_take(Bus::I2C);
        }
        ~IoLock()
        {
            if (_m._buses & BUS_MASK_I2C)
                bus_give(Bus::I2C);
            if (_m._buses & BUS_MASK_SPI)
                bus_give(Bus::SPI);
            _m.unlock_data();
        }
        IoLock(const IoLock &) = delete;
        IoLock &operator=(const IoLock &) = delete;

    private:
        Universal_Module &_m;
    };
};
