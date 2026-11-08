#include "UI_Home.h"
#include "RoboFaceWidget.h"

// The panel the app tiles sit in. The display is 480x480 and round, so this stays
// inside a ~220 px radius of the centre; tiles are placed by each app's iconPos()
// (y 200..290 for the default row).
static const zgfx::Rect OVERLAY_PANEL = {40, 150, 400, 170};

// The status panel (swipe down). Its top-left corner is the furthest point from the
// centre, ~227 px, so it stays inside the round display.
static const zgfx::Rect NOTIF_PANEL = {70, 90, 340, 256};

UI_Home::UI_Home(HAL_Robot_Module &robot, Robot_Procedure &procedure)
    : UI_Base(robot, procedure, "Home", {0, 0}),
      _overlay(zgfx::Rect{0, 0, BOARD_TFT_WIDTH, BOARD_TFT_HEIGHT}, OVERLAY_PANEL, "Apps", &UI_Home::s_selected, this),
      _notif(zgfx::Rect{0, 0, BOARD_TFT_WIDTH, BOARD_TFT_HEIGHT}, NOTIF_PANEL, robot, procedure)
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
    _notif.attach(_screen);
    _overlay.attach(_screen); // last: over the face, first to get touches
}

// Leaving for an app: put the panels away so they are closed when the face comes back.
void UI_Home::onExit()
{
    _overlay.close();
    _notif.close();
}

void UI_Home::refresh()
{
    _notif.refresh();

    if (!_faceWidget)
        return;

    // Freeze the face while a panel is up: no animation, so nothing repaints under it.
    if (_overlay.isOpen() || _notif.isOpen())
    {
        _wasCovered = true;
        return;
    }

    // Just closed (or back from an app): the blink/idle timers ran on while frozen, so
    // restart them instead of letting the face blink or move the instant it resumes.
    if (_wasCovered)
    {
        _wasCovered = false;
        _faceWidget->face().resetTimers();
    }
    _faceWidget->tick();
}

// Up: the app overlay (or dismisses the status panel). Down: the status panel (or
// dismisses the app overlay). Only one panel is open at a time.
void UI_Home::onSwipe(Swipe dir)
{
    if (_notif.isModal()) // calibrating: only the pop-up's buttons can end it
        return;
    switch (dir)
    {
    case Swipe::Up:
        if (_notif.isOpen())
            _notif.close();
        else
            _overlay.toggle();
        break;
    case Swipe::Down:
        if (_overlay.isOpen())
            _overlay.close();
        else
            _notif.open();
        break;
    default:
        break;
    }
}

void UI_Home::s_selected(void *self, UI_Base *app)
{
    static_cast<UI_Home *>(self)->launch(app);
}
