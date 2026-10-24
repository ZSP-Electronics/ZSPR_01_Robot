#include "ZSPR_Bottango_Driver.h"

#ifndef BOARD_MOTOR_RX
#define BOARD_MOTOR_RX 18
#endif
#ifndef BOARD_MOTOR_TX
#define BOARD_MOTOR_TX 19
#endif
// listed here so the library finder links these libraries to this one (Servo_Module.h is outside lib/)
#include <SCServo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Servo_Module.h"

ZSPR_BottangoDriver &ZSPR_BottangoDriver::instance()
{
    static ZSPR_BottangoDriver driver;
    return driver;
}

void ZSPR_BottangoDriver::begin(int signalOffset)
{
    if (_bus != nullptr)
    {
        return;
    }

    _signalOffset = signalOffset;
    _bus = new Servo_Module(true);
    _bus->setup();

    // Setup pings need replies; afterwards stop waiting for them so a missing servo
    // can never stall the Bottango loop.
    _bus->sc.Level = 0;
    _bus->st.Level = 0;
}

void ZSPR_BottangoDriver::servoWrite(uint8_t id, int signal)
{
    if (_bus == nullptr)
    {
        return;
    }

    int position = constrain(signal - _signalOffset, MIN_POSITION, MAX_POSITION);
    // time = 0, speed = 0: move at full speed, Bottango's curves provide the pacing
    _bus->writePosition(id, position, 0, 0);
}
