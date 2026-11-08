#pragma once

#include <Arduino.h>
#include "HAL_Robot_Module.h"
#include "Robot_Procedure.h"
#include "zGFX.h"

// Battery range for the bar and percentage readouts (Dashboard, notification panel).
// Adjust to the pack (this is one Li-ion cell).
#define UI_BATT_MIN_MV 3000
#define UI_BATT_MAX_MV 4200

// Base class for everything that owns a screen: the launcher and every app in
// UI/apps/. It carries the standard per-frame routine (touch -> refresh -> render)
// so a derived class only fills in the hooks:
//
//   onStart()  once, in the UI task: add widgets to _screen, set callbacks
//   onEnter()  each time the screen becomes the active one
//   onExit()   each time it stops being the active one
//   refresh()  every frame, before rendering: pull live data into the widgets
//   onSwipe()  optional: a finger dragged across the screen (see Swipe)
//   drawIcon() optional: paint the 64x64 launcher icon (or override icon())
//
// Everything runs in the UI task, so nothing here needs locking.
class UI_Base
{
public:
    static constexpr int ICON_SIZE = 64; // launcher icons are 64x64 RGB565

    // A swipe is reported once per touch, as soon as the finger has travelled
    // SWIPE_MIN_PX along one axis and at least twice as far as along the other.
    enum class Swipe : uint8_t
    {
        Up,
        Down,
        Left,
        Right
    };
    static constexpr int SWIPE_MIN_PX = 60;

    UI_Base(HAL_Robot_Module &robot, Robot_Procedure &procedure, const char *name, zgfx::Point iconPos)
        : _robot(robot), _procedure(procedure), _name(name), _iconPos(iconPos), _screen(BOARD_TFT_WIDTH, BOARD_TFT_HEIGHT, zgfx::colors::Black)
    {
    }
    virtual ~UI_Base() { free(_iconBuf); }

    const char *name() const { return _name; }
    // Top-left of the icon on the launcher screen.
    zgfx::Point iconPos() const { return _iconPos; }

    // 64x64 RGB565, row-major. The default renders drawIcon() into a buffer on
    // first use. An app with a ready-made bitmap can override this and return a
    // const array instead:  const uint16_t *icon() override { return my_icon; }
    virtual const uint16_t *icon();

    // ---- driven by UI_Task ---------------------------------------------
    void begin();                         // once, in the UI task
    void enter() { _screen.invalidateAll(); onEnter(); }
    void exit() { onExit(); }
    void touch(const TouchEvent &ev);     // display event -> widgets
    // Draws into fb if anything changed; true means present it. Always follow with endFrame().
    // scratch: optional internal-RAM strip the changed areas are drawn in before being
    // copied into fb (see zgfx::Screen::render()).
    bool frame(uint16_t *fb, uint16_t w, uint16_t h, uint16_t *scratch = nullptr, size_t scratchPixels = 0);
    void endFrame() { _screen.endFrame(); }

    // Navigation requests raised by widgets, collected by UI_Task after each event.
    UI_Base *takeLaunch() { UI_Base *a = _launch; _launch = nullptr; return a; }
    bool takeHome() { bool h = _home; _home = false; return h; }

protected:
    virtual void onStart() {}
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onSwipe(Swipe dir) { (void)dir; }
    virtual void refresh() {}
    virtual void drawIcon(zgfx::Canvas &c); // default: tile with the first letter of the name

    // Apps get a Home button (bottom centre); the launcher turns it off.
    virtual bool hasHomeButton() const { return true; }

    void goHome() { _home = true; }
    void launch(UI_Base *app) { _launch = app; }

    HAL_Robot_Module &_robot;
    Robot_Procedure &_procedure; // robot control: calibration, joint limits, joint targets
    zgfx::Screen _screen;

private:
    const char *_name;
    zgfx::Point _iconPos;
    uint16_t *_iconBuf = nullptr;
    UI_Base *_launch = nullptr;
    bool _home = false;

    // First finger down on this screen, for swipe detection.
    struct Gesture
    {
        bool active = false;
        bool fired = false;
        uint8_t id = 0;
        int16_t x0 = 0, y0 = 0;
    } _gesture;
    void detectSwipe(const TouchEvent &ev);

    // Bottom centre of the round panel; every corner is within ~220 px of the centre.
    zgfx::Button _homeBtn{{200, 416, 80, 40}, "Home"};
    static void s_homeClicked(void *self) { static_cast<UI_Base *>(self)->goHome(); }
};
