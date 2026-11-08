#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "HAL_Robot_Module.h"
#include "Robot_Procedure.h"
#include "UI_Base.h"
#include "UI_Home.h"

#define UI_SCRATCH_PIXELS (480 * 16) // internal-RAM draw strip: 15 KB, 16+ rows of any area

// 1: show a fixed test pattern instead of the UI (no animation, no touch), to tell
// scan-out faults (bands repeat or shift even though nothing is redrawn) from drawing
// bugs. Can also be set from platformio.ini with -DUI_TEST_PATTERN=1.
#ifndef UI_TEST_PATTERN
#define UI_TEST_PATTERN 0
#endif

class UI_Task
{
public:
    UI_Task(HAL_Robot_Module &robot, Robot_Procedure &procedure) : _robot(robot), _procedure(procedure), _home(robot, procedure) {}

    // Call once from setup(), after robot.init(). Does nothing (returns false)
    // if the display didn't come up.
    bool start();

private:
    HAL_Robot_Module &_robot;
    Robot_Procedure &_procedure;
    TaskHandle_t _task = nullptr;
    uint16_t _width = 0;
    uint16_t _height = 0;
    uint16_t *_scratch = nullptr; // UI_SCRATCH_PIXELS, internal RAM; null if it couldn't be allocated

    UI_Home _home;
    UI_Base *_active = nullptr;

    static void s_task(void *arg);
    void run(); // task body: never returns

    void clearBuffers();
    void runTestPattern(); // UI_TEST_PATTERN: draw once, then idle forever
    void switchTo(UI_Base *next);
    void handleNavigation();
};
