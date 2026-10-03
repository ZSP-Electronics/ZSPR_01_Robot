#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include "Servo_Module.h"
#include "BottangoLink.h"

// Bottango <-> servo bus. Adapts Servo_Module to the hardware-independent
// BottangoServoSink the BottangoLink library drives, and owns the lifecycle.
//
// Mapping (Bottango "I2C servo" = virtual PCA9685 channel):
//   channel      -> servo bus ID
//   pulse width  -> position (usMin..usMax -> servo range, see BottangoServoCal)
//   time between curve samples -> SC move time / ST speed
class Bottango_Module : public Universal_Module, private BottangoServoSink
{
public:
    Bottango_Module(bool enable, Servo_Module &servos) : Universal_Module(enable), _servos(servos) {}

    return_codes_t setup() override
    {
        if (!_enabled)
            return SUCCESS;
        BottangoServoBridge::setUpdateIntervalMs(BOTTANGO_UPDATE_MS);
        BottangoLink::begin(Serial, *this);
        return SUCCESS;
    }

    // Not driven from the HAL loop: update() is intentionally not overridden.
    // Blocks until the host sends "quit" on the USB port.
    void run()
    {
        if (_enabled)
            BottangoLink::run();
    }

    // Bus update period for streamed animation. 20 ms = 50 Hz.
    static constexpr uint16_t BOTTANGO_UPDATE_MS = 20;

private:
    // --- BottangoServoSink ---
    bool hasServo(uint8_t id) override { return _servos.servoTypeOf(id) >= 0; }

    bool servoRange(uint8_t id, int16_t &minPos, int16_t &maxPos) override
    {
        switch (_servos.servoTypeOf(id))
        {
        case SCSERVOTYPE:
            minPos = 20; // matches the angle limits Servo_Module::setMode(0) programs
            maxPos = 1003;
            return true;
        case STSERVOTYPE:
            minPos = 0;
            maxPos = 4095;
            return true;
        default:
            return false;
        }
    }

    void moveBatch(const BottangoServoMove *moves, uint8_t count) override
    {
        u8 scId[BOTTANGO_MAX_CHANNELS], stId[BOTTANGO_MAX_CHANNELS];
        u16 scPos[BOTTANGO_MAX_CHANNELS], scTime[BOTTANGO_MAX_CHANNELS], scSpeed[BOTTANGO_MAX_CHANNELS];
        s16 stPos[BOTTANGO_MAX_CHANNELS];
        u16 stSpeed[BOTTANGO_MAX_CHANNELS];
        u8 stAcc[BOTTANGO_MAX_CHANNELS];
        uint8_t nSc = 0, nSt = 0;

        for (uint8_t i = 0; i < count && i < BOTTANGO_MAX_CHANNELS; i++)
        {
            const BottangoServoMove &m = moves[i];
            switch (_servos.servoTypeOf(m.id))
            {
            case SCSERVOTYPE:
                scId[nSc] = m.id;
                scPos[nSc] = (u16)m.pos;
                scTime[nSc] = m.timeMs; // SC: duration governs speed, speed field left 0
                scSpeed[nSc] = 0;
                nSc++;
                break;
            case STSERVOTYPE:
                stId[nSt] = m.id;
                stPos[nSt] = m.pos;
                stSpeed[nSt] = m.speed == 0 ? ServoInitSpeed_ST : min<u16>(m.speed, ServoMaxSpeed_ST);
                stAcc[nSt] = 0; // no ramp: the curve itself is the motion profile
                nSt++;
                break;
            default:
                break;
            }
        }

        if (nSc)
            _servos.sc.SyncWritePos(scId, nSc, scPos, scTime, scSpeed);
        if (nSt)
            _servos.st.SyncWritePosEx(stId, nSt, stPos, stSpeed, stAcc);
    }

    Servo_Module &_servos;
};
