#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include "INA232.h"
#include "hardware_config.h"
#include "board_io.h"
#include "protocol_handlers.h"

#define SYSTEM_OHM 0.005f // 5mOhm shunt resistor for system current
#define MOTOR_OHM 0.005f  // 5mOhm shunt resistor
#define CURRENT_MAX 5.0f  // maximum expected current in Amps for calibration

class Current_Module : public Universal_Module
{
public:
    Current_Module(bool enable) : Universal_Module(enable)
    {
        currSys = new INA232(INA232_ADDR_A0_GND, Wire);  // system current, A0->GND
        currMotor = new INA232(INA232_ADDR_A0_VS, Wire); // motor current, A0->VS
    }

    return_codes_t setup() override
    {
        if (_enabled)
        {
            bool _ok1, _ok2;

            _ok1 = currSys->begin();
            currSys->setCalibration(SYSTEM_OHM, CURRENT_MAX);
            _ok2 = currMotor->begin();
            currMotor->setCalibration(MOTOR_OHM, CURRENT_MAX);

            if (!_ok1 || !_ok2)
            {
                Serial.println("INA232 not found");
                return ERROR;
            }
        }

        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            _sysVoltage_mV = currSys->readBusVoltage_mV();
            _sysCurrent_mA = currSys->readCurrent_mA();
            _sysPower_mW = currSys->readPower_mW();

            _motorVoltage_mV = currMotor->readBusVoltage_mV();
            _motorCurrent_mA = currMotor->readCurrent_mA();
            _motorPower_mW = currMotor->readPower_mW();
        }

        return SUCCESS;
    }

    return_codes_t getData(uint16_t *data, int argc, char **argv) override { return SUCCESS; }
    return_codes_t setData(uint16_t *data, int argc, char **argv) override { return SUCCESS; }

private:
    INA232 *currSys;
    INA232 *currMotor;

    uint16_t _sysVoltage_mV = 0;
    int16_t _sysCurrent_mA = 0;
    uint16_t _sysPower_mW = 0;

    uint16_t _motorVoltage_mV = 0;
    int16_t _motorCurrent_mA = 0;
    uint16_t _motorPower_mW = 0;

    uint8_t readCurrentSensor(INA232 *sensor, uint8_t *resp, uint8_t &respLen)
    {
        putU16(resp + 0, (uint16_t)sensor->readBusVoltage_mV());
        putS16(resp + 2, (int16_t)sensor->readCurrent_mA());
        putU16(resp + 4, (uint16_t)sensor->readPower_mW());
        return ok(respLen, 6);
    }

    uint8_t h_currSys(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
    {
        return readCurrentSensor(currSys, resp, respLen);
    }

    uint8_t h_currMotor(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
    {
        return readCurrentSensor(currMotor, resp, respLen);
    }
};