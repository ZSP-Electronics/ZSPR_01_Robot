#ifndef PinServoEffector_h
#define PinServoEffector_h

#include "LoopDrivenEffector.h"
#include "Arduino.h"
#ifdef ZSPR_SERVO_SHIM
#include "ZSPR_Servo_Shim.h" // ZSPR: Servo -> SC09 bus servo (lib/ZSPR_Bottango_Driver)
#elif defined(ESP32)
#include "lib/ESP32Servo/src/ESP32Servo.h"
#else
#include "Servo.h"
#endif

class PinServoEffector : public LoopDrivenEffector
{
public:
    PinServoEffector(byte pin, short minPWM, short maxPWM, int maxPWMSec, short startPWM);
    virtual void driveOnLoop() override;

    virtual void getIdentifier(char *outArray, short arraySize) override;
    virtual void destroy(bool systemShutdown) override;

protected:
private:
    byte pin = 0;
    Servo servo;
};

#endif