#pragma once

#include "../UI_Base.h"
#include "../widgets/UI_Dropdown.h"

// Changes a servo's bus ID: scan the bus, see which ID answers, pick the new ID
// from the drop-down and submit.
//
// Scanning pings IDs 0..MAX_SERVO_NUM-1 (the range Servo_Module scans at boot),
// one per frame, so the screen stays live; a silent ID costs up to the servo
// library's timeout. Only one servo may be on the bus when submitting (factory
// servos all share an ID, and two servos on one ID would both answer), so with
// several found the Set ID button stays disabled.
//
// Disabled while Bottango mode owns the servo bus.
class ServoId_App : public UI_Base
{
public:
    ServoId_App(HAL_Robot_Module &robot, Robot_Procedure &procedure) : UI_Base(robot, procedure, "Servo ID", {163, 200}) {}

protected:
    void onStart() override;
    void onExit() override;
    void refresh() override;
    void drawIcon(zgfx::Canvas &c) override;

private:
    enum class Scan : uint8_t
    {
        Idle,    // not scanned yet (or the result was invalidated)
        Running, // pinging _nextId
        Done
    };
    Scan _scan = Scan::Idle;
    uint8_t _nextId = 0;
    uint8_t _foundCount = 0;
    uint8_t _foundId[MAX_SERVO_NUM] = {};
    int _foundType[MAX_SERVO_NUM] = {};

    bool _blocked = false;      // Bottango owns the bus, or there is no servo bus
    bool _submitEnabled = true; // starts true so the first refresh() applies the disabled look
    char _result[zgfx::Label::MAX_TEXT] = "";

    bool busUsable() const;
    void startScan();
    void stepScan();
    void submit();
    void updateStatus();
    void setSubmitEnabled(bool on);
    static const char *typeName(int type);
    static void s_scanClicked(void *self) { static_cast<ServoId_App *>(self)->startScan(); }
    static void s_submitClicked(void *self) { static_cast<ServoId_App *>(self)->submit(); }

    // Layout for the round 480x480 panel (see Dashboard_App.h): every corner is
    // within ~225 px of (240,240). Text is the 5x7 font: scale 2 = 12x16, 3 = 18x24.
    // The Home button occupies y 416..456.
    zgfx::Label _title{{165, 44, 150, 24}, "Servo ID", zgfx::colors::White, zgfx::colors::Black, 3};
    zgfx::Button _scanBtn{{95, 88, 290, 48}, "Scan Bus"};
    zgfx::Label _status{{70, 150, 340, 16}, "Press Scan Bus", zgfx::colors::LightGray, zgfx::colors::Black, 2};
    zgfx::Label _resultLabel{{70, 174, 340, 16}, "", zgfx::colors::Yellow, zgfx::colors::Black, 2};
    zgfx::Button _submitBtn{{95, 364, 290, 44}, "Set ID"};
    // Added last, so its open grid (y 246..372) draws over the Set ID button and gets touches first.
    UI_Dropdown _newId{{95, 206, 290, 40}, "New ID: ", 0, MAX_SERVO_NUM, 4, 40};
};
