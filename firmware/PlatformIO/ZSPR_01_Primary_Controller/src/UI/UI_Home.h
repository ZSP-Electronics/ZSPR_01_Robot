#pragma once

#include "UI_Base.h"
#include "widgets/UI_Notifications.h"
#include "widgets/UI_Overlay.h"

class RoboFaceWidget; // lib/Robot_Face; its header stays out of here (it defines short global names)

// The primary screen: the animated robot face, full screen, shown at startup and
// whenever an app's Home button is pressed.
//
// Swiping up opens an overlay of app icons floating over the face; tap an
// icon to run that app, or swipe again / tap outside the panel to dismiss it.
// Swiping down opens a status panel (battery, rails, link, mode); swipe up or tap
// outside it to dismiss. Only one panel is open at a time, and the face is frozen
// while either is.
// Apps are registered here (see UI_Apps.h) and appear in the overlay at their iconPos().
class UI_Home : public UI_Base
{
public:
    UI_Home(HAL_Robot_Module &robot, Robot_Procedure &procedure);

    // Before begin(). Apps are shown in the overlay at their own iconPos().
    bool addApp(UI_Base *app) { return _overlay.addApp(app); }
    uint8_t appCount() const { return _overlay.appCount(); }
    UI_Base *app(uint8_t i) const { return _overlay.app(i); }

    // The face, for code that wants to drive it (expressions, colours...).
    // Null until begin() has run. Include RoboFaceWidget.h to use it.
    RoboFaceWidget *faceWidget() const { return _faceWidget; }

protected:
    void onStart() override;
    void onExit() override;
    void refresh() override;
    void onSwipe(Swipe dir) override;
    bool hasHomeButton() const override { return false; }

private:
    RoboFaceWidget *_faceWidget = nullptr;
    UI_Overlay _overlay;
    UI_Notifications _notif;
    bool _wasCovered = false; // a panel was over the face last frame

    static void s_selected(void *self, UI_Base *app);
};
