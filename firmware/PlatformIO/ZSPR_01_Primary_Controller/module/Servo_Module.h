#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include <esp_task_wdt.h>
#include <SCServo.h>
#include "hardware_config.h"
#include "board_io.h"

#define SCSERVOTYPE 5
#define STSERVOTYPE 9
#define MAX_SERVO_NUM 12

#define ServoInitACC_SC 0
#define ServoMaxSpeed_SC 1500
#define MaxSpeed_X_SC 1500
#define ServoInitSpeed_SC 800

#define ServoInitACC_ST 100
#define ServoMaxSpeed_ST 4000
#define MaxSpeed_X_ST 4000
#define ServoInitSpeed_ST 2000

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
                servoData[i].servoID = -1;
            }

            uint32_t start_time = millis();
            while (!Serial1)
            {

                if (millis() - start_time > 1000)
                    return ERROR;
            }

            int PingStatus, starting_index = 0;
            for (int i = 0; i < MAX_SERVO_NUM; i++)
            {
                PingStatus = st.Ping(i);
                if (PingStatus != -1)
                {
                    servoData[i].servoID = i;
                    servoData[i].ServoType = st.readByte(i, 3);
                    servoData[i].posRead = st.ReadPos(i);
                    foundServos++;
                    if (starting_index <= 0)
                        starting_index = i;
                }
                delay(1);
            }

#ifdef DEBUG
            Serial.print("ID:   ");
            for (int i = starting_index; i < starting_index + foundServos; i++)
            {
                Serial.printf("%2i ", servoData[i].servoID);
            }
            Serial.println();

            Serial.print("Type: ");
            for (int i = starting_index; i < starting_index + foundServos; i++)
            {
                Serial.printf("%2i ", servoData[i].ServoType);
            }
            Serial.println();

            Serial.println("Active Servos: " + String(foundServos));
#endif
            _initialized = true;
        }
        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            if (!_initialized)
                return ERROR;

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
#ifdef DEBUG
        else
        {
            Serial.println("Servo Type not found");
        }
#endif
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
    int MAX_MIN_OFFSET = 30;
    int foundServos = 0;

    // === SC Servo === TypeNum:5
    float ServoDigitalRange_SC = 1023.0;
    float ServoAngleRange_SC = 210.0;
    float ServoDigitalMiddle_SC = 511.0;

    // === ST Servo === TypeNum:9
    float ServoDigitalRange_ST = 4095.0;
    float ServoAngleRange_ST = 360.0;
    float ServoDigitalMiddle_ST = 2047.0;

    servo_data servoData[MAX_SERVO_NUM];
};