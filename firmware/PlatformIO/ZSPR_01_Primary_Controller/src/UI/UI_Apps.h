#pragma once

#include "UI_Home.h"

// ---------------------------------------------------------------------------
// App declarations: the one place that says which apps exist.
//
// To add an app:
//   1. Create UI/apps/My_App.h/.cpp with a class derived from UI_Base. Its
//      constructor passes the app's name and the icon's top-left position on the
//      overlay:   My_App(HAL_Robot_Module &r) : UI_Base(r, "Name", {x, y})
//      The icon is 64x64: paint it in drawIcon(), or return a ready-made RGB565
//      array from icon().
//   2. #include it below and add one UI_APP(...) line to UI_APP_LIST.
//
// The home screen's overlay (swipe up on the robot face) shows each app's
// icon and name at its position; tapping it switches to the app, and the app's
// Home button returns to the face.
//
// Positions: the panel is 480x480 and round (only the circle of radius 240 is
// visible). An icon tile is about 72 px wide and 100 px tall counting its name,
// so keep tiles inside a ~225 px radius of (240,240). Icon top-lefts for a
// 3-across row centred on the screen: {104,200}, {208,200}, {312,200}; for a
// 2-across row: {140,200}, {276,200}.
// ---------------------------------------------------------------------------

#include "apps/Dashboard_App.h"
#include "apps/ServoId_App.h"

// X(ClassName)
#define UI_APP_LIST(X) \
    X(Dashboard_App)   \
    X(ServoId_App)

// Creates every app in the list and registers it with the home screen. Returns the
// number registered. The apps live for the life of the program.
uint8_t ui_register_apps(HAL_Robot_Module &robot, UI_Home &home);
