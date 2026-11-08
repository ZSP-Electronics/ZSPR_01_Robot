#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include "INA232.h"
#include "hardware_config.h"
#include "board_io.h"

#define SYSTEM_OHM 0.005f // 5mOhm shunt resistor for system current
#define MOTOR_OHM 0.005f  // 5mOhm shunt resistor
#define CURRENT_MAX 16.0f // maximum expected current in Amps for calibration

// Property IDs for Current_Module::get(). Values are the readings cached by
// the last update().
enum class CurrentProp : uint16_t
{
    SysVoltage_mV,   // get INT
    SysCurrent_mA,   // get INT
    SysPower_mW,     // get INT
    MotorVoltage_mV, // get INT
    MotorCurrent_mA, // get INT
    MotorPower_mW,   // get INT
};

class Current_Module : public Universal_Module
{
public:
    INA232 *currSys;
    INA232 *currMotor;

    Current_Module(bool enable) : Universal_Module(enable)
    {
        currSys = new INA232(INA232_ADDR_A0_GND, Wire);  // system current, A0->GND
        currMotor = new INA232(INA232_ADDR_A0_VS, Wire); // motor current, A0->VS
    }

    return_codes_t setup() override
    {
        return_codes_t code = ERROR;
        if (_enabled)
        {
            bool _ok1, _ok2;

            _ok1 = currSys->begin();
            currSys->setCalibration(SYSTEM_OHM, CURRENT_MAX);
            _ok2 = currMotor->begin();
            currMotor->setCalibration(MOTOR_OHM, CURRENT_MAX);

            _initialized = _ok1 && _ok2;
            code = _initialized ? SUCCESS : ERROR;
        }

        return code;
    }

    return_codes_t update() override
    {
        if (!_enabled || !_initialized)
            return ERROR;

        _sysVoltage_mV = currSys->readBusVoltage_mV();
        _sysCurrent_mA = currSys->readCurrent_mA();
        _sysPower_mW = currSys->readPower_mW();

        _motorVoltage_mV = currMotor->readBusVoltage_mV();
        _motorCurrent_mA = currMotor->readCurrent_mA();
        _motorPower_mW = currMotor->readPower_mW();

        return SUCCESS;
    }

protected:
    return_codes_t onGet(uint16_t id, Value &out) override
    {
        if (!_initialized)
            return ERROR;
        switch ((CurrentProp)id)
        {
        case CurrentProp::SysVoltage_mV:   out = Value((int32_t)_sysVoltage_mV);   return SUCCESS;
        case CurrentProp::SysCurrent_mA:   out = Value((int32_t)_sysCurrent_mA);   return SUCCESS;
        case CurrentProp::SysPower_mW:     out = Value((int32_t)_sysPower_mW);     return SUCCESS;
        case CurrentProp::MotorVoltage_mV: out = Value((int32_t)_motorVoltage_mV); return SUCCESS;
        case CurrentProp::MotorCurrent_mA: out = Value((int32_t)_motorCurrent_mA); return SUCCESS;
        case CurrentProp::MotorPower_mW:   out = Value((int32_t)_motorPower_mW);   return SUCCESS;
        default:                           return NOT_SUPPORTED;
        }
    }

private:
    uint16_t _sysVoltage_mV = 0;
    int16_t _sysCurrent_mA = 0;
    uint16_t _sysPower_mW = 0;

    uint16_t _motorVoltage_mV = 0;
    int16_t _motorCurrent_mA = 0;
    uint16_t _motorPower_mW = 0;
};