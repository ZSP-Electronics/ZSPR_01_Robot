#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <AT42QT1070.h>
#include "Universal_Module.h"
#include "hardware_config.h"
#include "board_io.h"

// Property IDs for Cap_Touch_Module::get/set. The per-key props take a key
// index (0..6), so build the ID with Cap_Touch_Module::addr(prop, key); the
// rest ignore the key byte and can be passed as a plain CapTouchProp.
enum class CapTouchProp : uint8_t
{
    KeyCount,            // get UINT  number of keys on the chip (7)
    Touched,             // get UINT  bitmask, bit n = key n touched (as of last update())
    AnyTouched,          // get BOOL
    Calibrating,         // get BOOL
    Overflow,            // get BOOL  chip's cycle overflow flag
    KeyTouched,          // get BOOL  (key)
    KeySignal,           // get UINT  (key) live measurement
    KeyReference,        // get UINT  (key) live reference
    KeyThreshold,        // get/set UINT (key) detect threshold
    KeyAKSLevel,         // get/set UINT (key) adjacent key suppression group
    KeyIntegrator,       // get/set UINT (key) detection integrator count
    MeasurementInterval, // get/set UINT  low-power measurement interval
    MaxOnDuration,       // get/set UINT  max-on duration before auto-recalibration
};

enum class CapTouchCmd : uint8_t
{
    Calibrate, // force recalibration of all keys
    Reset,     // software reset of the chip
};

// AT42QT1070 7-key capacitive touch controller on the shared I2C bus.
//
// The CHANGE and RESET lines sit on the IO expander, which the AT42QT library
// can't drive (it uses native pinMode/digitalWrite). They are therefore not
// handed to the library: setup() configures them through board_io and the
// module polls the status register instead of waiting on CHANGE.
class Cap_Touch_Module : public Universal_Module
{
public:
    static constexpr uint8_t KEY_COUNT = AT42QT1070::KEY_COUNT;

    // Packs a property and a key index into the single 16-bit ID that
    // get/set take: low byte = prop, high byte = key.
    static constexpr uint16_t addr(CapTouchProp prop, uint8_t key) { return (uint16_t)prop | ((uint16_t)key << 8); }

    Cap_Touch_Module(bool enable = true) : Universal_Module(enable)
    {
        _buses = BUS_MASK_I2C; // AT42QT library uses Wire, unlocked
    }

    return_codes_t setup() override
    {
        return_codes_t code = ERROR;
        if (_enabled)
        {
            board_pinMode(BOARD_SENSE_CHG, INPUT); // open-drain, active-low from the chip
            board_pinMode(BOARD_SENSE_RST, OUTPUT);
            board_digitalWrite(BOARD_SENSE_RST, HIGH); // released

            // setup(), not begin(): begin() would re-run Wire.begin() with
            // default pins and undo the HAL's SDA/SCL assignment.
            _chip.setup(Wire);

            _initialized = _chip.communicating();
            if (_initialized)
            {
                read_status();
                code = SUCCESS;
            }
        }
        return code;
    }

    return_codes_t update() override
    {
        if (!_enabled || !_initialized)
            return ERROR;

        uint32_t now = millis();
        if (now - _lastPollMs >= POLL_INTERVAL_MS)
        {
            _lastPollMs = now;
            read_status();
        }
        return SUCCESS;
    }

    /*************************/
    /* CAP TOUCH ACCESSORS   */
    /*************************/
    uint8_t get_touched_mask(void) const { return _keys; }
    bool any_touched(void) const { return _keys != 0; }
    bool is_key_touched(uint8_t key) const { return key < KEY_COUNT && (_keys & (1u << key)); }
    bool is_calibrating(void) const { return _calibrating; }

protected:
    return_codes_t onGet(uint16_t id, Value &out) override
    {
        if (!_initialized)
            return ERROR;

        uint8_t key = (uint8_t)(id >> 8);

        switch ((CapTouchProp)(id & 0xFF))
        {
        case CapTouchProp::KeyCount:   out = Value((uint32_t)KEY_COUNT); return SUCCESS;
        case CapTouchProp::Touched:    out = Value((uint32_t)_keys);     return SUCCESS;
        case CapTouchProp::AnyTouched: out = Value(any_touched());       return SUCCESS;
        case CapTouchProp::Calibrating: out = Value(_calibrating);       return SUCCESS;
        case CapTouchProp::Overflow:   out = Value(_overflow);           return SUCCESS;
        default:
            break;
        }

        // Everything below is per-key.
        if (key >= KEY_COUNT)
            return INVALID_PARAM;

        switch ((CapTouchProp)(id & 0xFF))
        {
        case CapTouchProp::KeyTouched:   out = Value(is_key_touched(key));                           return SUCCESS;
        case CapTouchProp::KeySignal:    out = Value((uint32_t)_chip.getKeySignal(key));             return SUCCESS;
        case CapTouchProp::KeyReference: out = Value((uint32_t)_chip.getKeyReference(key));          return SUCCESS;
        case CapTouchProp::KeyThreshold: out = Value((uint32_t)_chip.getKeyDetectThreshold(key));    return SUCCESS;
        case CapTouchProp::KeyAKSLevel:  out = Value((uint32_t)_chip.getKeyAKSLevel(key));           return SUCCESS;
        case CapTouchProp::KeyIntegrator: out = Value((uint32_t)_chip.getKeyDetectIntegrator(key));  return SUCCESS;
        default:
            return NOT_SUPPORTED;
        }
    }

    return_codes_t onSet(uint16_t id, const Value &in) override
    {
        if (!_initialized)
            return ERROR;

        uint8_t key = (uint8_t)(id >> 8);
        uint8_t v = (uint8_t)constrain(in.asInt(), 0, 255);

        switch ((CapTouchProp)(id & 0xFF))
        {
        case CapTouchProp::MeasurementInterval:
            _chip.setMeasurementIntervalCount(v);
            return SUCCESS;
        case CapTouchProp::MaxOnDuration:
            _chip.setMaxOnDuration(v);
            return SUCCESS;
        case CapTouchProp::KeyThreshold:
        case CapTouchProp::KeyAKSLevel:
        case CapTouchProp::KeyIntegrator:
            if (key >= KEY_COUNT)
                return INVALID_PARAM;
            if ((CapTouchProp)(id & 0xFF) == CapTouchProp::KeyThreshold)
                _chip.setKeyDetectThreshold(key, v);
            else if ((CapTouchProp)(id & 0xFF) == CapTouchProp::KeyAKSLevel)
                _chip.setKeyAKSLevel(key, v);
            else
                _chip.setKeyDetectIntegrator(key, v);
            return SUCCESS;
        default:
            return NOT_SUPPORTED;
        }
    }

    return_codes_t onCommand(uint16_t id, const Value &in, Value &out) override
    {
        (void)in;
        (void)out;
        if (!_initialized)
            return ERROR;

        switch ((CapTouchCmd)(id & 0xFF))
        {
        case CapTouchCmd::Calibrate:
            _chip.triggerCalibration();
            return SUCCESS;
        case CapTouchCmd::Reset:
            _chip.reset();
            return SUCCESS;
        default:
            return NOT_SUPPORTED;
        }
    }

private:
    static constexpr uint32_t POLL_INTERVAL_MS = 10;

    AT42QT1070 _chip{Wire, -1, -1};

    uint8_t _keys = 0;
    bool _calibrating = false;
    bool _overflow = false;
    uint32_t _lastPollMs = 0;

    void read_status(void)
    {
        AT42QT1070::Status st = _chip.getStatus();
        _keys = (uint8_t)(st.bytes >> 8) & 0x7F; // keys field: bits 8..14
        _calibrating = (st.bytes & 0x80) != 0;
        _overflow = (st.bytes & 0x40) != 0;
    }
};
