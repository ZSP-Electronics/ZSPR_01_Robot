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
            float ax, ay, az, mx, my, mz;
            if (!readAccel(ax, ay, az) || !readMag(mx, my, mz))
                return TIMEOUT;

            float roll = atan2f(ay, az);
            float pitch = atan2f(-ax, ay * sinf(roll) + az * cosf(roll));

            float xh = mx * cosf(pitch) + mz * sinf(pitch);
            float yh = mx * sinf(roll) * sinf(pitch) + my * cosf(roll) - mz * sinf(roll) * cosf(pitch);

            float heading = atan2f(yh, xh) * 180.0f / (float)M_PI;
            if (heading < 0)
                heading += 360.0f;
        }
        return SUCCESS;
    }

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
    Adafruit_MMC5603 *_mag;
    bool imu_initialized = false;
    bool mag_initialized = false;

    float heading;

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