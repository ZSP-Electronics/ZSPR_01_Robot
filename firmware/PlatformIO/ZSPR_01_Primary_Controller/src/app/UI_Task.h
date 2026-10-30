#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "HAL_Robot_Module.h"

// The one task that draws. It owns the screen: nothing else may write to the
// frame buffers, so the GFX code needs no locking. Other tasks influence what is
// shown only through module state (read here with get()) or by posting to a
// queue/flag that this task polls.
//
//   touch  : Display task -> display->touchQueue() -> onTouch()
//   sensors: HAL peripheral task -> module cache -> get() in onFrame()
//   draw   : onFrame() into display->backBuffer(), then present()
#define UI_TASK_STACK_BYTES 8192
#define UI_TASK_PRIORITY 1 // below the HAL tasks: the screen must never delay sensor reads
#define UI_TASK_PERIOD_MS 33 // ~30 fps
#define UI_TASK_CORE 1

class UI_Task
{
public:
    explicit UI_Task(HAL_Robot_Module &robot) : _robot(robot) {}

    // Call once from setup(), after robot.init(). Does nothing (returns false)
    // if the display didn't come up.
    bool start();

private:
    HAL_Robot_Module &_robot;
    TaskHandle_t _task = nullptr;
    uint16_t _width = 0;
    uint16_t _height = 0;

    static void s_task(void *arg);
    void run(); // task body: never returns

    // ---- Application hooks: fill these in -------------------------------
    void onStart();                                          // once, before the first frame
    void onTouch(const TouchEvent &ev);                      // one per touch change
    void onFrame(uint16_t *fb, uint16_t w, uint16_t h);      // draw one frame into fb (RGB565)
};
