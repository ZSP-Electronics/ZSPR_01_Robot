#include "UI_Home.h"
#include "RoboFaceWidget.h"

// The panel the app tiles sit in. The display is 480x480 and round, so this stays
// inside a ~220 px radius of the centre; tiles are placed by each app's iconPos()
// (y 200..290 for the default row).
static const zgfx::Rect OVERLAY_PANEL = {40, 150, 400, 170};

UI_Home::UI_Home(HAL_Robot_Module &robot)
    : UI_Base(robot, "Home", {0, 0}),
      _overlay(zgfx::Rect{0, 0, BOARD_TFT_WIDTH, BOARD_TFT_HEIGHT}, OVERLAY_PANEL, "Apps", &UI_Home::s_selected, this)
{
}

void UI_Home::onStart()
{
    // Created here, in the UI task, so the face's state lives with the task that uses it.
    _faceWidget = new RoboFaceWidget(zgfx::Rect{0, 0, BOARD_TFT_WIDTH, BOARD_TFT_HEIGHT});
    RGBroboFace &face = _faceWidget->face();
    face.setEyeWidth(130);
    face.setEyeHeight(160);
    face.setEyeBorderradius(40);
    face.setEyeSpacebetween(20);
    face.setAutoblinker(true, 2, 4);
    face.setCuriosity(true);
    face.setPositionDistance(15);

    _screen.add(_faceWidget);
    _overlay.attach(_screen); // last: over the face, first to get touches
}

// Leaving for an app: put the overlay away so it is closed when the face comes back.
void UI_Home::onExit()
{
    _overlay.close();
}

void UI_Home::refresh()
{
    if (!_faceWidget)
        return;

    // Freeze the face while the overlay is up: no animation, so nothing repaints under it.
    const bool open = _overlay.isOpen();
    if (open)
    {
        _overlayWasOpen = true;
        return;
    }

    // Just closed (or back from an app): the blink/idle timers ran on while frozen, so
    // restart them instead of letting the face blink or move the instant it resumes.
    if (_overlayWasOpen)
    {
        _overlayWasOpen = false;
        _faceWidget->face().resetTimers();
    }
    _faceWidget->tick();
}

void UI_Home::onSwipe(Swipe dir)
{
    if (dir == Swipe::Up)
        _overlay.toggle();
}

void UI_Home::s_selected(void *self, UI_Base *app)
{
    static_cast<UI_Home *>(self)->launch(app);
}
