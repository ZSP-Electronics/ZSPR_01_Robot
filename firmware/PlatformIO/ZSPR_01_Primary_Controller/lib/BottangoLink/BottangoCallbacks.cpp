// Implements the Bottango driver's Callbacks:: hooks (upstream ships these as
// BottangoArduinoCallbacks.cpp, which is not vendored). Servo output needs no
// per-effector callback: the virtual PCA9685 receives every pulse directly.
#include "BottangoArduinoCallbacks.h"
#include "BottangoLink.h"

namespace Callbacks
{
    void onThisControllerStarted() { BottangoLink::_onControllerStarted(); }
    void onThisControllerStopped() { BottangoLink::_onControllerStopped(); }
    void onLateLoop() {}
    void onEarlyLoop() {}

    void onEffectorRegistered(AbstractEffector *) {}
    void onEffectorDeregistered(AbstractEffector *) {}
    void effectorSignalOnLoop(AbstractEffector *, int, bool) {}

    void onCurvedCustomEventMovementChanged(AbstractEffector *, float) {}
    void onOnOffCustomEventOnOffChanged(AbstractEffector *, bool) {}
    void onTriggerCustomEventTriggered(AbstractEffector *) {}
    void onColorCustomEventColorChanged(AbstractEffector *, byte, byte, byte) {}
    bool isEffectorAutoHomeComplete(AbstractEffector *, int &, int) { return false; }
    void onEffectorPostAutoHomeSecondarySyncComplete(AbstractEffector *) {}
    void onEffectorHomeReset(AbstractEffector *) {}
}
