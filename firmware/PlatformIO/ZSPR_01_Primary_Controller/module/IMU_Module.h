#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include "Arduino_BMI270_BMM150.h"
#include <Adafruit_MMC56x3.h>
#include "hardware_config.h"
#include "board_io.h"

#define IMU_ERROR_NONE 0x00
#define IMU_ERROR_ACCEL 0x01
#define IMU_ERROR_GYRO 0x02
#define IMU_ERROR_MAG 0x04
#define IMU_ERROR_HEADING 0x08

// Property IDs for IMU_Module::get(). Vector props return BYTES: 3 consecutive
// floats {x, y, z}, valid until the next update().
enum class ImuProp : uint16_t
{
    Accel,     // get BYTES  float[3] g
    Gyro,      // get BYTES  float[3] dps
    Mag,       // get BYTES  float[3] uT
    Heading,   // get FLOAT  tilt-compensated degrees
    Error,     // get UINT   IMU_ERROR_* bitmask
    AccelOk,   // get BOOL
    GyroOk,    // get BOOL
    MagOk,     // get BOOL
    HeadingOk, // get BOOL
};

class IMU_Module : public Universal_Module
{
public:
    Adafruit_MMC5603 *_mag;

    IMU_Module(bool enable) : Universal_Module(enable)
    {
        _mag = new Adafruit_MMC5603(12345);
        _buses = BUS_MASK_I2C; // BMI270 (global IMU) and MMC5603 both use Wire, unlocked
    }

    return_codes_t setup() override
    {
        if (_enabled)
        {
            imu_initialized = IMU.begin();
            mag_initialized = _mag->begin(MMC56X3_DEFAULT_ADDRESS, &Wire);
            if (!imu_initialized || !mag_initialized)
            {
                if (!imu_initialized)
                    _error |= IMU_ERROR_ACCEL;
                if (!mag_initialized)
                    _error |= IMU_ERROR_MAG;
                return ERROR;
            }
            else 
                _error = IMU_ERROR_NONE;
        }
        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            if (!imu_initialized || !mag_initialized)
                return ERROR;

            _accelOk = readAccel(_accel[0], _accel[1], _accel[2]);
            _gyroOk = readGyro(_gyro[0], _gyro[1], _gyro[2]);
            _magOk = readMag(_mag_v[0], _mag_v[1], _mag_v[2]);

            if (_accelOk && _magOk)
            {
                _heading = calculate_heading(_accel[0], _accel[1], _accel[2], _mag_v[0], _mag_v[1], _mag_v[2]);
                _headingOk = true;
                _error &= ~IMU_ERROR_HEADING;
            }
            else
            {
                _headingOk = false;
                _error |= IMU_ERROR_HEADING;
            }

            if (!_accelOk)
                _error |= IMU_ERROR_ACCEL;
            if (!_gyroOk)
                _error |= IMU_ERROR_GYRO;
            if (!_magOk)
                _error |= IMU_ERROR_MAG;

            if (!_accelOk || !_gyroOk || !_magOk)
                return TIMEOUT;
        }
        return SUCCESS;
    }

    uint8_t get_error(void)
    {
        return _error;
    }

    /*******************/
    /* IMU ACCESSORS   */
    /*******************/
    void get_accel(float &x, float &y, float &z) const { x = _accel[0]; y = _accel[1]; z = _accel[2]; }
    void get_gyro(float &x, float &y, float &z) const { x = _gyro[0]; y = _gyro[1]; z = _gyro[2]; }
    void get_mag(float &x, float &y, float &z) const { x = _mag_v[0]; y = _mag_v[1]; z = _mag_v[2]; }
    float get_heading(void) const { return _heading; }

    bool is_accel_ok(void) const { return _accelOk; }
    bool is_gyro_ok(void) const { return _gyroOk; }
    bool is_mag_ok(void) const { return _magOk; }
    bool is_heading_ok(void) const { return _headingOk; }

protected:
    return_codes_t onGet(uint16_t id, Value &out) override
    {
        switch ((ImuProp)id)
        {
        case ImuProp::Accel:     out = Value(_accel, sizeof(_accel)); return SUCCESS;
        case ImuProp::Gyro:      out = Value(_gyro, sizeof(_gyro));   return SUCCESS;
        case ImuProp::Mag:       out = Value(_mag_v, sizeof(_mag_v)); return SUCCESS;
        case ImuProp::Heading:   out = Value(_heading);               return SUCCESS;
        case ImuProp::Error:     out = Value((uint32_t)_error);       return SUCCESS;
        case ImuProp::AccelOk:   out = Value(_accelOk);               return SUCCESS;
        case ImuProp::GyroOk:    out = Value(_gyroOk);                return SUCCESS;
        case ImuProp::MagOk:     out = Value(_magOk);                 return SUCCESS;
        case ImuProp::HeadingOk: out = Value(_headingOk);             return SUCCESS;
        default:                 return NOT_SUPPORTED;
        }
    }

private:
    bool imu_initialized = false;
    bool mag_initialized = false;
    uint8_t _error = IMU_ERROR_NONE;

    float _accel[3] = {0, 0, 0};
    float _gyro[3] = {0, 0, 0};
    float _mag_v[3] = {0, 0, 0}; // _mag is already the MMC5603 driver
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