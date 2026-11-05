#include "ZSPR_Bottango_Driver.h"

// listed here so the library finder links these libraries to this one (Servo_Module.h is outside lib/)
#include <SCServo.h>
#include <Wire.h> // board_io.h -> KTS1622.h needs it; the finder only adds framework libs named here
#include <SPI.h>
#include "Servo_Module.h"

ZSPR_BottangoDriver &ZSPR_BottangoDriver::instance()
{
    static ZSPR_BottangoDriver driver;
    return driver;
}

void ZSPR_BottangoDriver::begin(Servo_Module &bus, int signalOffset)
{
    _signalOffset = signalOffset;
    _hostStopped = false;

    if (_bus == &bus)
    {
        return;
    }

    // Order matters: stop the servo task's polling first so nothing queues ahead
    // of the first animation write, then drop the reply waits.
    bus.pausePolling(true);
    bus.setAckWait(false);
    _bus = &bus;
}

void ZSPR_BottangoDriver::end()
{
    if (_bus == nullptr)
    {
        return;
    }

    Servo_Module *bus = _bus;
    _bus = nullptr; // writes become no-ops before the bus is handed back

    bus->setAckWait(true);
    bus->pausePolling(false);
}

void ZSPR_BottangoDriver::servoWrite(uint8_t id, int signal)
{
    Servo_Module *bus = _bus;
    if (bus == nullptr)
    {
        return;
    }

    int position = constrain(signal - _signalOffset, MIN_POSITION, MAX_POSITION);
    // time = 0, speed = 0: move at full speed, Bottango's curves provide the pacing
    bus->writePosition(id, position, 0, 0);
}
