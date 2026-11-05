#include "BottangoArduinoCallbacks.h"
#include "ZSPR_Bottango_Driver.h"

// Bottango callbacks. BottangoArduinoDriver's own BottangoArduinoCallbacks.cpp is not compiled
// in this layout (see its library.json), so these definitions satisfy the linker. Add
// behaviour here as needed.
namespace Callbacks
{
    void onThisControllerStarted() {}

    // The Bottango app sent "stop" (or we called BottangoCore::stop ourselves).
    // Bottango_Module sees the flag and leaves Bottango mode.
    void onThisControllerStopped() { ZSPR_BottangoDriver::instance().notifyHostStopped(); }
    void onEarlyLoop() {}
    void onLateLoop() {}

    void onEffectorRegistered(AbstractEffector *effector) {}
    void onEffectorDeregistered(AbstractEffector *effector) {}
    void effectorSignalOnLoop(AbstractEffector *effector, int signal, bool didChange) {}

    void onCurvedCustomEventMovementChanged(AbstractEffector *effector, float newMovement) {}
    void onOnOffCustomEventOnOffChanged(AbstractEffector *effector, bool on) {}
    void onTriggerCustomEventTriggered(AbstractEffector *effector) {}
    void onColorCustomEventColorChanged(AbstractEffector *effector, byte newRed, byte newGreen, byte newBlue) {}
    bool isEffectorAutoHomeComplete(AbstractEffector *effector, int &postAutoSyncMove, int autoSyncDirection) { return false; }
    void onEffectorPostAutoHomeSecondarySyncComplete(AbstractEffector *effector) {}
    void onEffectorHomeReset(AbstractEffector *effector) {}
} // namespace Callbacks
