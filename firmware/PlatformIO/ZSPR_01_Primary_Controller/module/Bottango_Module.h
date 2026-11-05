#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include "Servo_Module.h"
#include "peripheral_config.h"

#if ENABLE_BOTTANGO
#include "src/BottangoCore.h"
#include "ZSPR_Bottango_Driver.h"
#endif

// If the Bottango app has not completed its handshake this long after the mode
// was entered, give up and return to the CLI. Without it a robot left in
// Bottango mode with no app attached would sit there with the CLI muted.
#define BOTTANGO_CONNECT_TIMEOUT_MS 120000UL

// Property IDs for Bottango_Module::get/set.
enum class BottangoProp : uint16_t
{
    Active,    // get/set BOOL  the Bottango loop is running (set = request start/stop)
    Connected, // get BOOL      the app has completed its handshake
};

// Runs the Bottango driver loop while "active", on the servo bus owned by the
// HAL's Servo_Module. Does not know about the CLI: the serial port is shared
// with it, and handing the port over is the HAL's job (HAL_Robot_Module::set_mode),
// which is also why this module just reports through onStopped() when it ends
// by itself (app "stop", or the connect timeout).
//
// All BottangoCore calls are made from update(), i.e. from the Bottango task,
// because BottangoCore is not thread-safe. requestActive() only sets a flag.
class Bottango_Module : public Universal_Module
{
public:
    using StoppedFn = void (*)();

    Bottango_Module(bool enable, Servo_Module *servo)
        : Universal_Module(enable && ENABLE_BOTTANGO), _servo(servo) {}

    // Called from the Bottango task when the module leaves Bottango mode on its
    // own. Runs inside update(), so it must not wait on this module.
    void onStopped(StoppedFn fn) { _stoppedFn = fn; }

    // Lock-free; safe from any task. The change happens on the next update().
    void requestActive(bool on) { _requested = on; }
    bool requested() const { return _requested; }
    bool running() const { return _running; }

    return_codes_t setup() override
    {
        if (!_enabled || !_servo)
            return ERROR;
        _initialized = true; // BottangoCore itself is set up on first start
        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (!_enabled || !_initialized)
            return ERROR;

#if ENABLE_BOTTANGO
        if (_requested && !_running)
            start();
        else if (!_requested && _running)
            stop(false);

        if (!_running)
            return SUCCESS;

        BottangoCore::bottangoLoop();

        if (ZSPR_BottangoDriver::instance().hostStopped())
        {
            // The app sent "stop": BottangoCore already tore its effectors down.
            leave(true);
        }
        else if (!BottangoCore::initialized && millis() - _startedMs > BOTTANGO_CONNECT_TIMEOUT_MS)
        {
            leave(false);
        }
#endif
        return SUCCESS;
    }

protected:
    return_codes_t onGet(uint16_t id, Value &out) override
    {
        switch ((BottangoProp)id)
        {
        case BottangoProp::Active:
            out = Value(_running);
            return SUCCESS;
        case BottangoProp::Connected:
#if ENABLE_BOTTANGO
            out = Value(_running && BottangoCore::initialized);
#else
            out = Value(false);
#endif
            return SUCCESS;
        default:
            return NOT_SUPPORTED;
        }
    }

    return_codes_t onSet(uint16_t id, const Value &in) override
    {
        switch ((BottangoProp)id)
        {
        case BottangoProp::Active:
            requestActive(in.asBool());
            return SUCCESS;
        default:
            return NOT_SUPPORTED;
        }
    }

private:
    Servo_Module *_servo;
    StoppedFn _stoppedFn = nullptr;
    volatile bool _requested = false;
    volatile bool _running = false;

#if ENABLE_BOTTANGO
    bool _setupDone = false;
    uint32_t _startedMs = 0;

    void start()
    {
        ZSPR_BottangoDriver::instance().begin(*_servo);

        if (!_setupDone)
        {
            BottangoCore::bottangoSetup();
            _setupDone = true;
        }
        else
        {
            // Not a reboot: re-announce on the port the CLI just gave back.
            BottangoCore::initUSBSerialComms();
        }

        _startedMs = millis();
        _running = true;
    }

    // hostAlreadyStopped: BottangoCore.stop() has already run (the app asked).
    void stop(bool hostAlreadyStopped)
    {
        if (!hostAlreadyStopped)
            BottangoCore::stop(true); // deregisters the effectors, drains the port

        ZSPR_BottangoDriver &driver = ZSPR_BottangoDriver::instance();
        driver.end();
        driver.clearHostStopped();

        // uninitialize() clears `initialized` but not `handshake`, and the app's
        // next handshake is ignored while it is set (it expects a reboot).
        BottangoCore::handshake = false;

        _running = false;
    }

    // Ends Bottango mode from inside the task and tells the HAL.
    void leave(bool hostAlreadyStopped)
    {
        stop(hostAlreadyStopped);
        _requested = false;
        if (_stoppedFn)
            _stoppedFn();
    }
#endif
};
