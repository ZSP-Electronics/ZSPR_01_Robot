#pragma once

#include "../UI_Base.h"

// IMU axis conventions. The roll/pitch signs depend on how the board is mounted;
// if the horizon tilts the wrong way, flip the matching sign to -1. Heading is
// offset by UI_HEADING_OFFSET_DEG (add the mounting angle between the board's
// forward axis and the robot's).
// Mounting rotation (board upright vs flat) is IMU_MOUNT_ROTATE_DEG in IMU_Module.h.
// Dial smoothing: 0..1, fraction of the gap closed per frame. Higher follows
// faster but is noisier; 1 disables smoothing.
#define UI_IMU_SMOOTHING 0.5f
#define UI_ROLL_SIGN 1
#define UI_PITCH_SIGN 1
#define UI_HEADING_OFFSET_DEG 0.0f

// Battery, current, IMU dials and the Bottango mode switch.
class Dashboard_App : public UI_Base
{
public:
    Dashboard_App(HAL_Robot_Module &robot, Robot_Procedure &procedure) : UI_Base(robot, procedure, "Status", {73, 200}) {}

protected:
    void onStart() override;
    void refresh() override;
    void drawIcon(zgfx::Canvas &c) override;

private:
    // Smoothed IMU values shown on the dials.
    float _fPitch = 0, _fRoll = 0, _fHdg = 0;
    bool _fAttSeen = false, _fHdgSeen = false;

    void refreshImu(); // dials and readout from the IMU
    void toggleBottango();
    static void s_bottangoClicked(void *self);

    // 480x480 round panel: only the circle of radius 240 around (240,240) is
    // visible. Layout keeps every widget corner inside a 230 px radius (10 px
    // margin). For a box spanning y0..y1 the widest it may be, centred, is
    // 2*sqrt(230^2 - max(|y0-240|,|y1-240|)^2); the rows below are within that.
    // Text is the 5x7 font at an integer scale: scale 2 is a 12x16 cell,
    // scale 3 is 18x24. The Home button occupies y 416..456 at the bottom.

    // "ZSPR-01" at scale 3 is 123 px wide; x = 240 - 123/2.
    zgfx::Label _title{{178, 48, 126, 24}, "ZSPR-01", zgfx::colors::White, zgfx::colors::Black, 3};

    // Rows span x 70..410.
    zgfx::Label _battLabel{{70, 100, 130, 16}, "Battery", zgfx::colors::LightGray, zgfx::colors::Black, 2};
    zgfx::Label _battValue{{230, 100, 180, 16}, "---", zgfx::colors::White, zgfx::colors::Black, 2};
    zgfx::ProgressBar _battBar{{70, 124, 340, 20}, UI_BATT_MIN_MV, UI_BATT_MAX_MV};

    zgfx::Label _currLabel{{70, 172, 130, 16}, "Current", zgfx::colors::LightGray, zgfx::colors::Black, 2};
    zgfx::Label _currValue{{230, 172, 180, 16}, "---", zgfx::colors::White, zgfx::colors::Black, 2};

    // IMU dials, 96 px each, centres 75 px either side of the vertical axis
    // (furthest point ~121 px from the screen centre).
    zgfx::Horizon _horizon{{117, 198, 96, 96}};
    zgfx::Compass _compass{{267, 198, 96, 96}};
    zgfx::Label _imuValue{{70, 300, 340, 16}, "---", zgfx::colors::White, zgfx::colors::Black, 2};

    zgfx::Label _modeLabel{{70, 322, 340, 16}, "Mode: CLI", zgfx::colors::LightGray, zgfx::colors::Black, 2};
    zgfx::Button _bottangoBtn{{95, 346, 290, 60}, "Bottango"};
};
