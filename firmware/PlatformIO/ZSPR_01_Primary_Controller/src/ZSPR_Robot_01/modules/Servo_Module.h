#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include <esp_task_wdt.h>
#include <SCServo.h>
#include "hardware_config.h"
#include "board_io.h"
#include "NVS_Store.h"

#define SCSERVOTYPE 5
#define STSERVOTYPE 9

#define ServoInitACC_SC 0
#define ServoMaxSpeed_SC 1500
#define MaxSpeed_X_SC 1500
#define ServoInitSpeed_SC 800

#define ServoInitACC_ST 100
#define ServoMaxSpeed_ST 4000
#define MaxSpeed_X_ST 4000
#define ServoInitSpeed_ST 2000

#define ST_SERVO_ID_LEFT    1
#define ST_SERVO_ID_RIGHT   2
#define FR_SERVO_ID         3   // Front Right
#define FL_SERVO_ID         4   // Front Left
#define AER_SERVO_ID        8   // Arm Extension Right
#define ASR_SERVO_ID        9   // Arm Shoulder Right
#define ARR_SERVO_ID        7   // Arm Rotate Right
#define AEL_SERVO_ID        5   // Arm Extension Left
#define ASL_SERVO_ID        6   // Arm Shoulder Left
#define ARL_SERVO_ID        10  // Arm Rotate Left
#define EAR_SERVO_ID        12  // Eye Arm Right
#define EAL_SERVO_ID        11  // Eye Arm Left

struct servo_data
{
    int servoID;
    int ServoType;
    int16_t loadRead;
    int16_t speedRead;
    byte voltageRead;
    int16_t currentRead;
    int16_t posRead;
    int16_t modeRead;
    int16_t temperRead;
    bool Torque_List;
    uint16_t targetSpeed = 1000; // used by the next Position set
    uint8_t targetAcc = 0;
};

// Property IDs for Servo_Module::get/set. Servo properties are per-servo, so
// build the ID with Servo_Module::addr(prop, busId).
enum class ServoProp : uint8_t
{
    Count,    // get  UINT  number of servos found at setup() (busId ignored)
    IdAt,     // get  UINT  bus ID of the n-th found servo (n = the busId byte)
    Position, // get/set INT  raw position (set uses the servo's Speed/Acc)
    Speed,    // get/set INT  get: measured speed; set: cruise speed for the next Position set
    Acc,      // set UINT     acceleration for the next Position set
    Load,     // get  INT
    Voltage,  // get  UINT
    Current,  // get  INT
    Temp,     // get  INT
    Mode,     // get/set INT  set: 0 = servo, 1/2 = closed/open loop wheel, 3 = step (see setMode)
    Torque,   // set  BOOL    torque enable
};

enum class ServoCmd : uint8_t
{
    Stop,      // release and re-enable torque
    SetMiddle, // ST servos: store current position as middle
    SetId,     // in: new bus ID (UINT)
};

class Servo_Module : public Universal_Module
{
public:
    SCSCL sc;
    SMS_STS st;


    // Packs a property and a servo bus ID into the single 16-bit ID that
    // get/set/command take: low byte = prop, high byte = servo ID.
    static constexpr uint16_t addr(ServoProp prop, uint8_t servoId) { return (uint16_t)prop | ((uint16_t)servoId << 8); }

    Servo_Module(bool enable) : Universal_Module(enable)
    {
    }

    return_codes_t setup() override
    {
        if (_enabled)
        {
            foundServos = 0;
            Serial1.begin(1000000, SERIAL_8N1, BOARD_MOTOR_RX, BOARD_MOTOR_TX);
            sc.pSerial = &Serial1;
            st.pSerial = &Serial1;

            for (int i = 0; i < MAX_SERVO_NUM; i++)
            {
                servoData[i].Torque_List = true;
                servoData[i].ServoType = -1;
                // servoData[i].servoID = -1;
            }

            uint32_t start_time = millis();
            while (!Serial1)
            {

                if (millis() - start_time > 1000)
                    return ERROR;
            }

            int PingStatus;
            for (int i = 0; i < MAX_SERVO_NUM; i++)
            {
                PingStatus = st.Ping(i);
                if (PingStatus != -1)
                {
                    servoData[i].servoID = i;
                    servoData[i].ServoType = st.readByte(i, 3);
                    servoData[i].posRead = st.ReadPos(i);
                    foundServos++;
                }
                delay(1);
            }

            // A list saved with saveServoIds() replaces the scanned/default IDs.
            loadServoIds();

            _initialized = true;
        }
        return SUCCESS;
    }

    // Lets the Bottango driver own the bus while it animates: feedback polling
    // stops (every poll would queue ahead of an animation write on the bus
    // lock), and writes stop waiting for the servos' replies (a missing servo
    // would otherwise cost IOTimeOut per write and stall the animation).
    // Both are lock-free / bus-lock-only so they are safe to call from any task
    // while the servo task is mid-pass.
    void pausePolling(bool pause) { _pollPaused = pause; }

    void setAckWait(bool wait)
    {
        BusLock bus(Bus::Servo); // Level is only read inside bus transactions
        sc.Level = wait ? 1 : 0;
        st.Level = wait ? 1 : 0;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            if (!_initialized)
                return ERROR;

            if (_pollPaused)
                return SUCCESS;

            for (int i = 0; i < foundServos; i++)
            {
                getFeedBack(servoData[i].servoID);
                // A silent servo costs the library's IOTimeOut per read, so a
                // pass over many dead servos can outlast the task watchdog.
                // No-op if the calling task isn't subscribed.
                esp_task_wdt_reset();
            }
        }

        return SUCCESS;
    }


    /*******************/
    /* SERVO FUNCTIONS */
    /*******************/
    void getFeedBack(byte servoID)
    {
        DataLock lock(*this);
        byte InputIDIndex = 0;

        for (int i = 0; i < foundServos; i++)
        {
            if (servoData[i].servoID == servoID)
            {
                InputIDIndex = i;
                break;
            }
        }

        if (servoData[InputIDIndex].ServoType == STSERVOTYPE)
        {
            if (st.FeedBack(servoID) != -1)
            {
                servoData[InputIDIndex].posRead = st.ReadPos(servoID);
                servoData[InputIDIndex].speedRead = st.ReadSpeed(servoID);
                servoData[InputIDIndex].loadRead = st.ReadLoad(servoID);
                servoData[InputIDIndex].voltageRead = st.ReadVoltage(servoID);
                servoData[InputIDIndex].currentRead = st.ReadCurrent(servoID);
                servoData[InputIDIndex].temperRead = st.ReadTemper(servoID);
                servoData[InputIDIndex].modeRead = st.ReadMode(servoID);
            }
        }
        else if (servoData[InputIDIndex].ServoType == SCSERVOTYPE)
        {
            if (sc.FeedBack(servoID) != -1)
            {
                servoData[InputIDIndex].posRead = sc.ReadPos(servoID);
                servoData[InputIDIndex].speedRead = sc.ReadSpeed(servoID);
                servoData[InputIDIndex].loadRead = sc.ReadLoad(servoID);
                servoData[InputIDIndex].voltageRead = sc.ReadVoltage(servoID);
                servoData[InputIDIndex].currentRead = sc.ReadCurrent(servoID);
                servoData[InputIDIndex].temperRead = sc.ReadTemper(servoID);
                servoData[InputIDIndex].modeRead = sc.ReadMode(servoID);
            }
        }
    }

    void setMiddle(byte InputID)
    {
        DataLock lock(*this);
        byte InputIDIndex = 0;

        for (int i = 0; i < foundServos; i++)
        {
            if (servoData[i].servoID == InputID)
            {
                InputIDIndex = i;
                break;
            }
        }

        if (servoData[InputIDIndex].ServoType == STSERVOTYPE)
        {
            st.CalibrationOfs(InputID);
        }
    }

    void setMode(byte InputID, byte InputMode)
    {
        DataLock lock(*this);
        byte InputIDIndex = 0;

        for (int i = 0; i < foundServos; i++)
        {
            if (servoData[i].servoID == InputID)
            {
                InputIDIndex = i;
                break;
            }
        }

        if (InputMode == 0)
        {
            if (servoData[InputIDIndex].ServoType == STSERVOTYPE)
            {
                st.unLockEprom(InputID);
                st.writeWord(InputID, 11, 4095);
                st.writeByte(InputID, SMS_STS_MODE, InputMode);
                st.LockEprom(InputID);
            }
            else if (servoData[InputIDIndex].ServoType == SCSERVOTYPE)
            {
                sc.unLockEprom(InputID);
                sc.writeWord(InputID, SCSCL_MIN_ANGLE_LIMIT_L, 20);
                sc.writeWord(InputID, SCSCL_MAX_ANGLE_LIMIT_L, 1003);
                sc.LockEprom(InputID);
            }
        }

        if (InputMode == 1 || InputMode == 2)
        {
            if (servoData[InputIDIndex].ServoType == STSERVOTYPE)
            {
                st.unLockEprom(InputID);
                // st.writeWord(InputID, 11, 4095);
                st.writeByte(InputID, SMS_STS_MODE, InputMode);
                st.LockEprom(InputID);
            }
            // else if(ServoType[InputIDIndex] == SCSERVOTYPE){
            //   sc.unLockEprom(InputID);
            //   sc.writeWord(InputID, SCSCL_MIN_ANGLE_LIMIT_L, 20);
            //   sc.writeWord(InputID, SCSCL_MAX_ANGLE_LIMIT_L, 1003);
            //   sc.LockEprom(InputID);
            // }
        }

        else if (InputMode == 3)
        {
            if (servoData[InputIDIndex].ServoType == STSERVOTYPE)
            {
                st.unLockEprom(InputID);
                st.writeByte(InputID, SMS_STS_MODE, InputMode);
                st.writeWord(InputID, 11, 0);
                st.LockEprom(InputID);
            }
            else if (servoData[InputIDIndex].ServoType == SCSERVOTYPE)
            {
                sc.unLockEprom(InputID);
                sc.writeWord(InputID, SCSCL_MIN_ANGLE_LIMIT_L, 0);
                sc.writeWord(InputID, SCSCL_MAX_ANGLE_LIMIT_L, 0);
                sc.LockEprom(InputID);
            }
        }
    }

    int setID(byte ID_select, byte ID_set)
    {
        DataLock lock(*this);
        int error = 0;
        if (ID_set > MAX_SERVO_NUM)
        {
            ID_set = MAX_SERVO_NUM;
        }

        if (servoData[ID_select].ServoType == STSERVOTYPE)
        {
            st.unLockEprom(ID_select);
            error = st.writeByte(ID_select, SMS_STS_ID, ID_set);
            st.LockEprom(ID_set);
        }
        else if (servoData[ID_select].ServoType == SCSERVOTYPE)
        {
            sc.unLockEprom(ID_select);
            error = sc.writeByte(ID_select, SCSCL_ID, ID_set);
            sc.LockEprom(ID_set);
        }

        return error;
    }

    // Pings one bus ID. Returns the servo type (SCSERVOTYPE / STSERVOTYPE) of
    // whatever answered, or -1 if nothing did. Leaves the tracked-servo table
    // alone, so it is safe for scanning. A silent ID costs the library's
    // IOTimeOut; call it from a task that can afford that.
    int probe(uint8_t id)
    {
        if (!_initialized)
            return -1;
        if (st.Ping(id) == -1)
            return -1;
        return st.readByte(id, 3);
    }

    // Changes the bus ID of the servo answering at oldId to newId (0..MAX_SERVO_NUM-1,
    // the range setup() scans) and keeps the polling table in step. Works on a
    // servo that was not found at boot too.
    // INVALID_PARAM: bad or identical ID.  ERROR: newId is taken, oldId doesn't
    // answer, or the write didn't take.
    return_codes_t changeId(uint8_t oldId, uint8_t newId)
    {
        if (!_initialized)
            return ERROR;
        if (oldId >= MAX_SERVO_NUM || newId >= MAX_SERVO_NUM || oldId == newId)
            return INVALID_PARAM;

        int type = probe(oldId);
        if (type != SCSERVOTYPE && type != STSERVOTYPE)
            return ERROR;
        if (probe(newId) != -1) // two servos on one ID would both answer
            return ERROR;

        if (type == STSERVOTYPE)
        {
            st.unLockEprom(oldId);
            st.writeByte(oldId, SMS_STS_ID, newId);
            st.LockEprom(newId); // it answers to the new ID from here on
        }
        else
        {
            sc.unLockEprom(oldId);
            sc.writeByte(oldId, SCSCL_ID, newId);
            sc.LockEprom(newId);
        }

        // Confirm by asking the new ID rather than trusting the write's ack
        // (a half-duplex bus can drop it).
        if (probe(newId) == -1)
            return ERROR;

        DataLock lock(*this);
        int idx = indexOf(oldId);
        if (idx < 0 && foundServos < MAX_SERVO_NUM)
        {
            idx = foundServos++;
            servoData[idx].Torque_List = true;
        }
        if (idx >= 0)
        {
            servoData[idx].servoID = newId;
            servoData[idx].ServoType = type;
        }
        return SUCCESS;
    }

    // Replaces the bus ID held by each servoData[] slot from an array: ids[i] goes to
    // slot i (slot order is the order of the ID defines; Robot_Procedure names them); slots at or past `count` are left
    // alone. Only this module's table changes: the servos themselves are not
    // reprogrammed (changeId() does that), so the new IDs must already be the ones the
    // servos answer to.
    // INVALID_PARAM: null array, count > MAX_SERVO_NUM, an ID above MAX_SERVO_NUM, or the
    // same ID twice. The table is left untouched in that case.
    return_codes_t setServoIds(const uint8_t *ids, uint8_t count)
    {
        if (!ids || count > MAX_SERVO_NUM)
            return INVALID_PARAM;

        uint32_t used = 0; // bit n set: ID n is taken (IDs run 0..MAX_SERVO_NUM)
        for (uint8_t i = 0; i < count; i++)
        {
            if (ids[i] > MAX_SERVO_NUM || (used & (1UL << ids[i])))
                return INVALID_PARAM;
            used |= 1UL << ids[i];
        }

        DataLock lock(*this);
        for (uint8_t i = 0; i < count; i++)
            servoData[i].servoID = ids[i];
        return SUCCESS;
    }

    // NVS key for the saved ID list (one byte per slot, in slot order).
    static constexpr const char *NVS_KEY_SERVO_IDS = "servo_ids";

    // Writes the table's current IDs to NVS so setup() restores them at the next boot.
    bool saveServoIds()
    {
        uint8_t ids[MAX_SERVO_NUM];
        {
            DataLock lock(*this);
            for (uint8_t i = 0; i < MAX_SERVO_NUM; i++)
                ids[i] = (uint8_t)servoData[i].servoID;
        }
        return NVS::saveArray(NVS_KEY_SERVO_IDS, ids, MAX_SERVO_NUM);
    }

    // Applies the list saved by saveServoIds(). False (table unchanged) if nothing is
    // saved or the saved list is no longer valid.
    bool loadServoIds()
    {
        uint8_t ids[MAX_SERVO_NUM];
        if (!NVS::loadArray(NVS_KEY_SERVO_IDS, ids, MAX_SERVO_NUM))
            return false;
        return setServoIds(ids, MAX_SERVO_NUM) == SUCCESS;
    }

    // Forgets the saved list; the next boot goes back to the scanned/default IDs.
    bool clearSavedServoIds() { return NVS::erase(NVS_KEY_SERVO_IDS); }

    /*********************/
    /* PER-SLOT ACCESS   */
    /*********************/
    // Primitives addressed by servoData[] slot (not by bus ID), for the robot procedure
    // layer: it owns the slot names, calibration and limits. Slot order is the order of
    // the ID defines above. All are safe from any task.

    // Servo type (SCSERVOTYPE / STSERVOTYPE) found in the slot, or -1 if none.
    int servoTypeAt(uint8_t slot) const
    {
        if (slot >= MAX_SERVO_NUM)
            return -1;
        DataLock lock(*this);
        return servoData[slot].ServoType;
    }

    // Current position read straight from the servo, or -1 if it didn't answer.
    int readPositionAt(uint8_t slot)
    {
        if (slot >= MAX_SERVO_NUM)
            return -1;
        DataLock lock(*this);
        const servo_data &s = servoData[slot];
        if (s.ServoType == STSERVOTYPE)
            return st.ReadPos(s.servoID);
        if (s.ServoType == SCSERVOTYPE)
            return sc.ReadPos(s.servoID);
        return -1;
    }

    void writePositionAt(uint8_t slot, int16_t pos, uint16_t speed, uint8_t acc)
    {
        if (slot >= MAX_SERVO_NUM)
            return;
        DataLock lock(*this);
        const servo_data &s = servoData[slot];
        if (s.ServoType == STSERVOTYPE)
            st.WritePosEx(s.servoID, pos, speed, acc);
        else if (s.ServoType == SCSERVOTYPE)
            sc.WritePos(s.servoID, pos, speed, acc);
    }

    void torqueAt(uint8_t slot, bool on)
    {
        if (slot >= MAX_SERVO_NUM)
            return;
        DataLock lock(*this);
        setTorqueSlot(slot, on);
        servoData[slot].Torque_List = on;
    }

    void servoStop(byte servoID)
    {
        DataLock lock(*this);
        uint8_t _index = 0;

        for (int i = 0; i < foundServos; i++)
        {
            if (servoData[i].servoID == servoID)
            {
                _index = i;
                break;
            }
        }

        if (servoData[_index].ServoType == STSERVOTYPE)
        {
            st.EnableTorque(servoID, 0);
            delay(10);
            st.EnableTorque(servoID, 1);
        }
        else if (servoData[_index].ServoType == SCSERVOTYPE)
        {
            sc.EnableTorque(servoID, 0);
            delay(10);
            sc.EnableTorque(servoID, 1);
        }
    }

    // void servoPWM(byte servoID, s16 PWM) {
    //   if (ServoType[servoID] == STSERVOTYPE) {
    //     st.WritePWM(servoID, PWM);
    //   } else if (ServoType[servoID] == SCSERVOTYPE) {
    //     sc.WritePWM(servoID, PWM);
    //   }
    // }

    void servoTorque(byte servoID, u8 enableCMD)
    {
        DataLock lock(*this);
        uint8_t _index = 0;

        for (int i = 0; i < foundServos; i++)
        {
            if (servoData[i].servoID == servoID)
            {
                _index = i;
                break;
            }
        }

        if (servoData[_index].ServoType == STSERVOTYPE)
        {
            st.EnableTorque(servoID, enableCMD);
        }
        else if (servoData[_index].ServoType == SCSERVOTYPE)
        {
            sc.EnableTorque(servoID, enableCMD);
        }
    }

    void writePosition(uint8_t id, int16_t pos, uint16_t speed = 1000, uint8_t acc = 0)
    {
        DataLock lock(*this);
        uint8_t _index = 0;

        for (int i = 0; i < foundServos; i++)
        {
            if (servoData[i].servoID == id)
            {
                _index = i;
                break;
            }
        }

        if (servoData[_index].ServoType == STSERVOTYPE)
        {
            st.WritePosEx(id, pos, speed, acc);
        }
        else if (servoData[_index].ServoType == SCSERVOTYPE)
        {
            sc.WritePos(id, pos, speed, acc);
        }
    }

    // Servo type (SCSERVOTYPE / STSERVOTYPE) of the servo with this bus ID,
    // or -1 if it wasn't found at setup().
    int servoTypeOf(uint8_t id) const
    {
        DataLock lock(*this);
        for (int i = 0; i < MAX_SERVO_NUM; i++)
        {
            if (servoData[i].servoID == id)
                return servoData[i].ServoType;
        }
        return -1;
    }

    uint8_t get_found_servos(void)
    {
        DataLock lock(*this);
        return foundServos;
    }

    int get_servo_id(uint8_t index)
    {
        DataLock lock(*this);
        return servoData[index].servoID;
    }

    struct Feedback
    {
        int16_t pos;
        int16_t speed;
        int16_t load;
        uint8_t voltage;
        int16_t temp;
    };

    // Most recently cached feedback for servo `id` (as of the last
    // getFeedBack() call for that id, made every update()). False if `id`
    // isn't one of the currently-tracked servoData[] slots.
    bool get_feedback(uint8_t id, Feedback &out) const
    {
        DataLock lock(*this);
        for (int i = 0; i < foundServos; i++)
        {
            if (servoData[i].servoID == id)
            {
                out.pos = servoData[i].posRead;
                out.speed = servoData[i].speedRead;
                out.load = servoData[i].loadRead;
                out.voltage = servoData[i].voltageRead;
                out.temp = servoData[i].temperRead;
                return true;
            }
        }
        return false;
    }

protected:
    // Index into servoData[] of the servo with this bus ID, or -1.
    int indexOf(uint8_t servoId) const
    {
        for (int i = 0; i < foundServos; i++)
        {
            if (servoData[i].servoID == servoId)
                return i;
        }
        return -1;
    }

    return_codes_t onGet(uint16_t id, Value &out) override
    {
        if (!_initialized)
            return ERROR;

        ServoProp prop = (ServoProp)(id & 0xFF);
        if (prop == ServoProp::Count)
        {
            out = Value((uint32_t)foundServos);
            return SUCCESS;
        }
        if (prop == ServoProp::IdAt)
        {
            uint8_t n = (uint8_t)(id >> 8);
            if (n >= foundServos)
                return INVALID_PARAM;
            out = Value((uint32_t)servoData[n].servoID);
            return SUCCESS;
        }

        int idx = indexOf((uint8_t)(id >> 8));
        if (idx < 0)
            return INVALID_PARAM;
        const servo_data &s = servoData[idx];

        switch (prop)
        {
        case ServoProp::Position: out = Value((int32_t)s.posRead);     return SUCCESS;
        case ServoProp::Speed:    out = Value((int32_t)s.speedRead);   return SUCCESS;
        case ServoProp::Load:     out = Value((int32_t)s.loadRead);    return SUCCESS;
        case ServoProp::Voltage:  out = Value((uint32_t)s.voltageRead); return SUCCESS;
        case ServoProp::Current:  out = Value((int32_t)s.currentRead); return SUCCESS;
        case ServoProp::Temp:     out = Value((int32_t)s.temperRead);  return SUCCESS;
        case ServoProp::Mode:     out = Value((int32_t)s.modeRead);    return SUCCESS;
        default:                  return NOT_SUPPORTED;
        }
    }

    return_codes_t onSet(uint16_t id, const Value &in) override
    {
        if (!_initialized)
            return ERROR;

        uint8_t servoId = (uint8_t)(id >> 8);
        int idx = indexOf(servoId);
        if (idx < 0)
            return INVALID_PARAM;
        servo_data &s = servoData[idx];

        switch ((ServoProp)(id & 0xFF))
        {
        case ServoProp::Position:
            writePosition(servoId, (int16_t)in.asInt(), s.targetSpeed, s.targetAcc);
            return SUCCESS;
        case ServoProp::Speed:
            s.targetSpeed = (uint16_t)constrain(in.asInt(), 0, 65535);
            return SUCCESS;
        case ServoProp::Acc:
            s.targetAcc = (uint8_t)constrain(in.asInt(), 0, 255);
            return SUCCESS;
        case ServoProp::Mode:
            setMode(servoId, (byte)in.asInt());
            return SUCCESS;
        case ServoProp::Torque:
            servoTorque(servoId, in.asBool() ? 1 : 0);
            s.Torque_List = in.asBool();
            return SUCCESS;
        default:
            return NOT_SUPPORTED;
        }
    }

    return_codes_t onCommand(uint16_t id, const Value &in, Value &out) override
    {
        (void)out;
        if (!_initialized)
            return ERROR;

        uint8_t servoId = (uint8_t)(id >> 8);
        if (indexOf(servoId) < 0)
            return INVALID_PARAM;

        switch ((ServoCmd)(id & 0xFF))
        {
        case ServoCmd::Stop:
            servoStop(servoId);
            return SUCCESS;
        case ServoCmd::SetMiddle:
            setMiddle(servoId);
            return SUCCESS;
        case ServoCmd::SetId:
            return setID(servoId, (byte)in.asInt()) == 0 ? SUCCESS : ERROR;
        default:
            return NOT_SUPPORTED;
        }
    }

private:
    // Torque for a servoData[] slot directly (servoTorque() looks the servo up by ID).
    // Caller holds the data lock.
    void setTorqueSlot(uint8_t slot, bool on)
    {
        const servo_data &s = servoData[slot];
        if (s.ServoType == STSERVOTYPE)
            st.EnableTorque(s.servoID, on ? 1 : 0);
        else if (s.ServoType == SCSERVOTYPE)
            sc.EnableTorque(s.servoID, on ? 1 : 0);
    }

    int MAX_MIN_OFFSET = 30;
    int foundServos = 0;
    volatile bool _pollPaused = false;

    // === SC Servo === TypeNum:5
    float ServoDigitalRange_SC = 1023.0;
    float ServoAngleRange_SC = 210.0;
    float ServoDigitalMiddle_SC = 511.0;

    // === ST Servo === TypeNum:9
    float ServoDigitalRange_ST = 4095.0;
    float ServoAngleRange_ST = 360.0;
    float ServoDigitalMiddle_ST = 2047.0;

    servo_data servoData[MAX_SERVO_NUM] = {
        {ST_SERVO_ID_LEFT},
        {ST_SERVO_ID_RIGHT},
        {FR_SERVO_ID},
        {FL_SERVO_ID},
        {AER_SERVO_ID},
        {ASR_SERVO_ID},
        {ARR_SERVO_ID},
        {AEL_SERVO_ID},
        {ASL_SERVO_ID},
        {ARL_SERVO_ID},
        {EAR_SERVO_ID},
        {EAL_SERVO_ID}
    };
};