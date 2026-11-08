#pragma once

#include "../UI_Base.h"
#include "Robot_Procedure.h" // servo slot names, limit capture

// Sets each joint's minimum and maximum position by hand. Opening the app turns every
// joint's torque off so the horns move freely. Move a joint to one end of its travel and
// press its Min button, to the other end and press Max: each press reads the servo and
// records the position in the robot procedure's pending limits. Leaving the app (the Home
// button) saves the min and max lists to NVS and turns torque back on.
//
// Joints are the servos Robot_Procedure calibrates (the arm and eye servos, found on the bus
// and SC-type); a row for any other servo is greyed out. Positions are in calibrated units.
// See Robot_Procedure::limitCaptureBegin().
class JointLimits_App : public UI_Base
{
public:
    JointLimits_App(HAL_Robot_Module &robot, Robot_Procedure &procedure)
        : UI_Base(robot, procedure, "Limits", {343, 200}) {}

protected:
    void onStart() override;
    void onEnter() override;
    void onExit() override;
    void drawIcon(zgfx::Canvas &c) override;

private:
    static constexpr uint8_t FIRST = SERVO_FIRST_CONFIGURABLE_SLOT;
    static constexpr uint8_t COUNT = SERVO_CAL_COUNT; // rows shown

    struct Row;
    struct Cell
    {
        JointLimits_App *owner = nullptr;
        uint8_t index = 0; // 0..COUNT-1
        bool isMax = false;
    };
    struct Row
    {
        zgfx::Label *name = nullptr;
        zgfx::Button *minBtn = nullptr;
        zgfx::Button *maxBtn = nullptr;
        Cell minCell, maxCell;
    };
    Row _rows[COUNT];

    bool _active = false; // limit capture is running (torque off)

    void capture(uint8_t index, bool isMax);
    void setButtonLook(zgfx::Button *b, bool enabled, bool captured);
    void setMessage(const char *text, zgfx::color_t color);
    static void s_clicked(void *cell);

    // Layout for the round 480x480 panel (see Dashboard_App.h): every corner is within
    // ~230 px of (240,240). Text is the 5x7 font: scale 2 = 12x16. The Home button
    // occupies y 416..456.
    static constexpr int ROW_Y0 = 88;    // top of the first row
    static constexpr int ROW_PITCH = 28; // rows are 26 px tall
    static constexpr int NAME_X = 70;    // "Arm Extension Right" at scale 2 is 228 px
    static constexpr int MIN_X = 304;
    static constexpr int MAX_X = 358;
    static constexpr int BTN_W = 52;

    zgfx::Label _title{{156, 40, 168, 16}, "Joint Limits", zgfx::colors::White, zgfx::colors::Black, 2};
    zgfx::Label _status{{70, 378, 340, 16}, "", zgfx::colors::LightGray, zgfx::colors::Black, 2};
};
