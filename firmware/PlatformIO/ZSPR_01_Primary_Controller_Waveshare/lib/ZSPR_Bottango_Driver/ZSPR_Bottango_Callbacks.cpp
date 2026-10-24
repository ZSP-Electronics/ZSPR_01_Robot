#include "BottangoArduinoCallbacks.h"

// Bottango callbacks. BottangoArduinoDriver's own BottangoArduinoCallbacks.cpp is not compiled
// in this layout, so these definitions satisfy the linker. Add behaviour here as needed.
namespace Callbacks
{
    void onThisControllerStarted() {}
    void onThisControllerStopped() {}
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
