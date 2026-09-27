#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include <SCServo.h>
#include "hardware_config.h"
#include "board_io.h"
#include "protocol_handlers.h"

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
};

class Servo_Module : public Universal_Module
{
public:
    SCSCL sc;
    SMS_STS st;

    Servo_Module(bool enable) : Universal_Module(enable)
    {
    }

    return_codes_t setup() override
    {
        if (_enabled)
        {
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
                {
#ifdef DEBUG
                    Serial.println("Serial1 not ready after 1 second");
#endif
                    return ERROR;
                }
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
        }
        return SUCCESS;
    }

    return_codes_t update() override
    {
        if (_enabled)
        {
            for (int i = 0; i < foundServos; i++)
            {
                getFeedBack(servoData[i].servoID);
            }
        }

        return SUCCESS;
    }

    return_codes_t getData(uint16_t *data, int argc, char **argv) override
    {
        return SUCCESS;
    }

    return_codes_t setData(uint16_t *data, int argc, char **argv) override
    {
        return SUCCESS;
    }

    /*******************/
    /* SERVO FUNCTIONS */
    /*******************/
    void getFeedBack(byte servoID)
    {
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

    uint8_t get_found_servos(void)
    {
        return foundServos;
    }

    int get_servo_id(uint8_t index)
    {
        return servoData[index].servoID;
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