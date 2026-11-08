#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include "Arduino_BMI270_BMM150.h"
#include <Adafruit_MMC56x3.h>
#include "hardware_config.h"
#include "board_io.h"

// How the board is mounted. Roll and pitch assume the board's Z axis points up
// when the robot is level; if the board stands upright instead, rotate about Y by
// a quarter turn so that gravity lands on Z again. 0 = board flat, 90 / 270 =
// board upright (try the other one if roll and pitch come out mirrored), 180 =
// upside down. The magnetometer is rotated the same way so heading stays
// tilt-compensated in the mounted frame. Raw Accel/Gyro/Mag props are unrotated.
#ifndef IMU_MOUNT_ROTATE_DEG
#define IMU_MOUNT_ROTATE_DEG 90
#endif

// Magnetometer output rate (free-running). Samples older than IMU_STALE_MS count
// as a sensor fault.
#ifndef IMU_MAG_RATE_HZ
#define IMU_MAG_RATE_HZ 100
#endif
#define IMU_STALE_MS 250

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
    Roll,      // get FLOAT  degrees, from accel (valid while AccelOk)
    Pitch,     // get FLOAT  degrees, from accel (valid while AccelOk)
};

class IMU_Module : public Universal_Module
{
public:
    Adafruit_MMC5603 *_mag;

    IMU_Module(bool enable) : Universal_Module(enable)
    {
        _mag = new Adafruit_MMC5603(12345);
        _buses = BUS_MASK_I2C; // BMI270 (global IMU) and MMC5603 both use Wire, unlocked
        _get_locks_buses = false; // every get() returns a value cached by update()
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

            // Free-running magnetometer: getEvent() then just reads registers.
            // One-shot mode (the library default) triggers a measurement and polls
            // with delay(5), blocking ~10 ms per update with the I2C bus held.
            _mag->setDataRate(IMU_MAG_RATE_HZ);
            _mag->setContinuousMode(true);
        }
        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            if (!imu_initialized || !mag_initialized)
                return ERROR;

            // update() runs faster than the sensors produce samples, so most calls
            // find nothing new. That is not a fault: keep the last values and call
            // a sensor "ok" while its last sample is recent. (Flagging every empty
            // poll as a failure made the UI flicker between valid and invalid.)
            const uint32_t now = millis();
            bool newAccel = readAccel(_accel[0], _accel[1], _accel[2]);
            if (newAccel)
                _accelMs = now;
            if (readGyro(_gyro[0], _gyro[1], _gyro[2]))
                _gyroMs = now;
            if (readMag(_mag_v[0], _mag_v[1], _mag_v[2]))
                _magMs = now;

            _accelOk = fresh(_accelMs, now);
            _gyroOk = fresh(_gyroMs, now);
            _magOk = fresh(_magMs, now);

            // Attitude and heading are computed in the mounted frame, so accel and
            // mag are rotated identically before use.
            float ax, ay, az, mx, my, mz;
            mount_rotate(_accel, ax, ay, az);
            mount_rotate(_mag_v, mx, my, mz);

            if (_accelOk && newAccel)
            {
                float roll, pitch;
                calculate_attitude(ax, ay, az, roll, pitch);
                _roll = roll * 180.0f / (float)M_PI;
                _pitch = pitch * 180.0f / (float)M_PI;
            }

            if (_accelOk && _magOk)
            {
                _heading = calculate_heading(ax, ay, az, mx, my, mz);
                _headingOk = true;
            }
            else
            {
                _headingOk = false;
            }

            _error = IMU_ERROR_NONE;
            if (!_accelOk)
                _error |= IMU_ERROR_ACCEL;
            if (!_gyroOk)
                _error |= IMU_ERROR_GYRO;
            if (!_magOk)
                _error |= IMU_ERROR_MAG;
            if (!_headingOk)
                _error |= IMU_ERROR_HEADING;

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
        case ImuProp::Roll:      out = Value(_roll);                  return SUCCESS;
        case ImuProp::Pitch:     out = Value(_pitch);                 return SUCCESS;
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
    float _roll = 0;
    float _pitch = 0;
    uint32_t _accelMs = 0; // millis() of the last new sample; 0 = none yet
    uint32_t _gyroMs = 0;
    uint32_t _magMs = 0;
    bool _accelOk = false;
    bool _gyroOk = false;
    bool _magOk = false;
    bool _headingOk = false;

    // Tilt-compensated compass heading (degrees) from raw accel + mag readings.
    static bool fresh(uint32_t sampleMs, uint32_t now)
    {
        return sampleMs != 0 && (now - sampleMs) < IMU_STALE_MS;
    }

    // Rotate a sensor vector about Y by IMU_MOUNT_ROTATE_DEG (quarter turns only).
    // At 90, a reading of +1 g on the board's X axis becomes +1 g on Z (level).
    static void mount_rotate(const float v[3], float &x, float &y, float &z)
    {
        y = v[1];
#if IMU_MOUNT_ROTATE_DEG == 0
        x = v[0];
        z = v[2];
#elif IMU_MOUNT_ROTATE_DEG == 90
        x = -v[2];
        z = v[0];
#elif IMU_MOUNT_ROTATE_DEG == 180
        x = -v[0];
        z = -v[2];
#elif IMU_MOUNT_ROTATE_DEG == 270
        x = v[2];
        z = -v[0];
#else
#error IMU_MOUNT_ROTATE_DEG must be 0, 90, 180 or 270
#endif
    }

    // Roll and pitch in radians from the gravity vector.
    static void calculate_attitude(float ax, float ay, float az, float &roll, float &pitch)
    {
        roll = atan2f(ay, az);
        pitch = atan2f(-ax, ay * sinf(roll) + az * cosf(roll));
    }

    float calculate_heading(float ax, float ay, float az, float mx, float my, float mz)
    {
        float roll, pitch;
        calculate_attitude(ax, ay, az, roll, pitch);

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