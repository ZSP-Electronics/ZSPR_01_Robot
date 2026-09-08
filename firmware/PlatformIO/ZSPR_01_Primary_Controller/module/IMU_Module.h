#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include "Arduino_BMI270_BMM150.h"
#include <Adafruit_MMC56x3.h>
#include "hardware_config.h"
#include "board_io.h"
#include "protocol_handlers.h"

class IMU_Module : public Universal_Module
{
public:
    Adafruit_MMC5603 *_mag;

    IMU_Module(bool enable) : Universal_Module(enable)
    {
        _mag = new Adafruit_MMC5603(12345);
    }

    return_codes_t setup() override
    {
        if (_enabled)
        {
            imu_initialized = IMU.begin();
            mag_initialized = _mag->begin(MMC56X3_DEFAULT_ADDRESS, &Wire);
            if (!imu_initialized || !mag_initialized)
            {
                return ERROR;
            }
        }
        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            _accelOk = readAccel(_ax, _ay, _az);
            _gyroOk = readGyro(_gx, _gy, _gz);
            _magOk = readMag(_mx, _my, _mz);

            if (_accelOk && _magOk)
            {
                _heading = calculate_heading(_ax, _ay, _az, _mx, _my, _mz);
                _headingOk = true;
            }
            else
            {
                _headingOk = false;
            }

            if (!_accelOk || !_magOk)
                return TIMEOUT;
        }
        return SUCCESS;
    }

    /*******************/
    /* IMU ACCESSORS   */
    /*******************/
    void get_accel(float &x, float &y, float &z) const { x = _ax; y = _ay; z = _az; }
    void get_gyro(float &x, float &y, float &z) const { x = _gx; y = _gy; z = _gz; }
    void get_mag(float &x, float &y, float &z) const { x = _mx; y = _my; z = _mz; }
    float get_heading(void) const { return _heading; }

    bool is_accel_ok(void) const { return _accelOk; }
    bool is_gyro_ok(void) const { return _gyroOk; }
    bool is_mag_ok(void) const { return _magOk; }
    bool is_heading_ok(void) const { return _headingOk; }

    return_codes_t getData(uint16_t *data, int argc, char **argv) override
    {
        if (_enabled)
        {
        }
        return SUCCESS;
    }

    return_codes_t setData(uint16_t *data, int argc, char **argv) override
    {
        if (_enabled)
        {
            // Implement any setData functionality if needed
        }
        return SUCCESS;
    }

private:
    bool imu_initialized = false;
    bool mag_initialized = false;

    float _ax = 0, _ay = 0, _az = 0;
    float _gx = 0, _gy = 0, _gz = 0;
    float _mx = 0, _my = 0, _mz = 0;
    float _heading = 0;
    bool _accelOk = false;
    bool _gyroOk = false;
    bool _magOk = false;
    bool _headingOk = false;

    // Tilt-compensated compass heading (degrees) from raw accel + mag readings.
    float calculate_heading(float ax, float ay, float az, float mx, float my, float mz)
    {
        float roll = atan2f(ay, az);
        float pitch = atan2f(-ax, ay * sinf(roll) + az * cosf(roll));

        float xh = mx * cosf(pitch) + mz * sinf(pitch);
        float yh = mx * sinf(roll) * sinf(pitch) + my * cosf(roll) - mz * sinf(roll) * cosf(pitch);

        float hdg = atan2f(yh, xh) * 180.0f / (float)M_PI;
        if (hdg < 0)
            hdg += 360.0f;
        return hdg;
    }

    bool readAccel(float &x, float &y, float &z)
    {
        if (!IMU.accelerationAvailable())
            return false;
        return IMU.readAcceleration(x, y, z) != 0;
    }

    bool readGyro(float &x, float &y, float &z)
    {
        if (!IMU.gyroscopeAvailable())
            return false;
        return IMU.readGyroscope(x, y, z) != 0;
    }

    bool readMag(float &x, float &y, float &z)
    {
        sensors_event_t event;
        if (!_mag->getEvent(&event))
            return false;
        x = event.magnetic.x;
        y = event.magnetic.y;
        z = event.magnetic.z;
        return true;
    }
};