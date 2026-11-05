#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "HAL_Robot_Module.h"
#include "UI_Base.h"
#include "UI_Home.h"

// The one task that draws. It owns the screen: nothing else may write to the
// frame buffers, so ZGFX needs no locking. Other tasks influence what is shown
// only through module state (read with get()) or by posting to a queue/flag that
// this task polls.
//
// The task itself is generic. What is on screen is the active UI_Base: the
// home screen (the robot face, with the app overlay) or one of the apps declared
// in UI_Apps.h.
//
//   touch  : Display task -> display->touchQueue() -> active->touch()
//   draw   : active->frame() into display->backBuffer(), then present()
//   switch : overlay icon / app Home button -> takeLaunch()/takeHome() here
#define UI_TASK_STACK_BYTES 8192
#define UI_TASK_PRIORITY 1 // below the HAL tasks: the screen must never delay sensor reads
#define UI_TASK_PERIOD_MS 20 // 50 fps cap; a changing frame also waits ~2 vsyncs in present()
#define UI_TASK_CORE 1

// 1: show a fixed test pattern instead of the UI (no animation, no touch), to tell
// scan-out faults (bands repeat or shift even though nothing is redrawn) from drawing
// bugs. Can also be set from platformio.ini with -DUI_TEST_PATTERN=1.
#ifndef UI_TEST_PATTERN
#define UI_TEST_PATTERN 0
#endif

class UI_Task
{
public:
    explicit UI_Task(HAL_Robot_Module &robot) : _robot(robot), _home(robot) {}

    // Call once from setup(), after robot.init(). Does nothing (returns false)
    // if the display didn't come up.
    bool start();

private:
    HAL_Robot_Module &_robot;
    TaskHandle_t _task = nullptr;
    uint16_t _width = 0;
    uint16_t _height = 0;

    UI_Home _home;
    UI_Base *_active = nullptr;

    static void s_task(void *arg);
    void run(); // task body: never returns

    void clearBuffers();
    void runTestPattern(); // UI_TEST_PATTERN: draw once, then idle forever
    void switchTo(UI_Base *next);
    void handleNavigation();
};
