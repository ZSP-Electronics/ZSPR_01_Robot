#pragma once

#include "../UI_Base.h"
#include "Robot_Procedure.h" // servo slot names
#include "../widgets/UI_Dropdown.h"

// Robot servo configuration: one row per servo (the names come from the ID defines in
// Servo_Module.h), with a drop-down beside each showing the bus ID the module currently
// holds for it. Change any of them, then press Submit to hand the whole list to
// Servo_Module::setServoIds().
//
// The two ST wheel servos are not listed; their IDs are passed through unchanged.
// Submit updates the module's table and saves the list to NVS (Servo_Module::saveServoIds),
// so it is restored at boot. It does not reprogram the servos themselves (the Servo ID
// app does that).
// Two servos can't share an ID: Submit refuses and says which one is repeated.
class ServoConfig_App : public UI_Base
{
public:
    ServoConfig_App(HAL_Robot_Module &robot, Robot_Procedure &procedure) : UI_Base(robot, procedure, "Servos", {253, 200}) {}

protected:
    void onStart() override;
    void onEnter() override;
    void onExit() override;
    void refresh() override;
    void drawIcon(zgfx::Canvas &c) override;

private:
    static constexpr uint8_t FIRST = SERVO_FIRST_CONFIGURABLE_SLOT;
    static constexpr uint8_t COUNT = MAX_SERVO_NUM - FIRST; // rows shown

    // Drop-down IDs run 0..MAX_SERVO_NUM: the range the module accepts.
    static constexpr int ID_FIRST = 0;
    static constexpr int ID_COUNT = MAX_SERVO_NUM + 1;

    struct Row
    {
        ServoConfig_App *owner = nullptr;
        uint8_t index = 0; // 0..COUNT-1
        zgfx::Label *name = nullptr;
        UI_Dropdown *drop = nullptr;
    };
    Row _rows[COUNT];
    uint8_t _passthroughId[FIRST] = {}; // the ST servos' IDs, read when the screen opens

    bool _blocked = false;      // no servo module
    bool _submitEnabled = true; // starts true so the first refresh() applies the disabled look

    void load();
    void submit();
    void rowToggled(uint8_t index, bool open);
    void setSubmitEnabled(bool on);
    void setMessage(const char *text, zgfx::color_t color);
    static void s_toggled(void *row, bool open);
    static void s_submitClicked(void *self) { static_cast<ServoConfig_App *>(self)->submit(); }

    // Layout for the round 480x480 panel (see Dashboard_App.h): every corner is within
    // ~230 px of (240,240). Text is the 5x7 font: scale 2 = 12x16, 3 = 18x24.
    // The Home button occupies y 416..456.
    static constexpr int ROW_Y0 = 88;     // top of the first row
    static constexpr int ROW_PITCH = 28;  // rows are 26 px tall
    static constexpr int NAME_X = 84;     // "Arm Extension Right" at scale 2 is 228 px
    static constexpr int BOX_X = 316;     // drop-down boxes are 80 x 26

    zgfx::Label _title{{168, 40, 144, 16}, "Servo Config", zgfx::colors::White, zgfx::colors::Black, 2};
    zgfx::Label _status{{110, 62, 260, 16}, "", zgfx::colors::LightGray, zgfx::colors::Black, 2};
    zgfx::Button _submitBtn{{120, 376, 240, 34}, "Submit"};
};
