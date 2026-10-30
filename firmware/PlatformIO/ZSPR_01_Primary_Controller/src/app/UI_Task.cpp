#include "UI_Task.h"
#include <esp_task_wdt.h>
#include <cstring>

bool UI_Task::start()
{
    if (!_robot.display || !_robot.display->initialized())
    {
        Serial.println("[ui] display not available, UI task not started");
        return false;
    }

    BaseType_t created = xTaskCreatePinnedToCore(&UI_Task::s_task, "ui", UI_TASK_STACK_BYTES, this,
                                                 UI_TASK_PRIORITY, &_task, UI_TASK_CORE);
    if (created != pdPASS)
    {
        Serial.println("[ui] failed to start UI task");
        return false;
    }
    return true;
}

void UI_Task::s_task(void *arg)
{
    static_cast<UI_Task *>(arg)->run();
}

void UI_Task::run()
{
    Display_Module *display = _robot.display;

    if (esp_task_wdt_add(nullptr) != ESP_OK)
    {
        Serial.println("[ui] task could not join the task watchdog");
    }

    {
        Value v;
        _width = display->get(DisplayProp::Width, v) == SUCCESS ? (uint16_t)v.asUInt() : 0;
        _height = display->get(DisplayProp::Height, v) == SUCCESS ? (uint16_t)v.asUInt() : 0;
    }

    onStart();

    QueueHandle_t touchQ = display->touchQueue();
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(UI_TASK_PERIOD_MS);

    while (1)
    {
        // Input first, so the frame reflects it.
        TouchEvent ev;
        while (touchQ && xQueueReceive(touchQ, &ev, 0) == pdTRUE)
        {
            onTouch(ev);
        }

        uint16_t *fb = display->backBuffer();
        if (fb)
        {
            onFrame(fb, _width, _height);
            display->present(); // waits for vsync, so this also paces the loop
        }

        esp_task_wdt_reset();

        if (xTaskDelayUntil(&lastWake, period) == pdFALSE)
        {
            vTaskDelay(1);
            lastWake = xTaskGetTickCount();
        }
    }
}

// ---------------------------------------------------------------------------
// Application hooks
// ---------------------------------------------------------------------------

void UI_Task::onStart()
{
    // Both buffers start undefined; clear them so the first present() isn't noise.
    Display_Module *display = _robot.display;
    for (uint8_t i = 0; i < display->frameBufferCount(); i++)
    {
        uint16_t *fb = display->frameBuffer(i);
        if (fb)
            memset(fb, 0, (size_t)_width * _height * sizeof(uint16_t));
    }

    // Create fonts, load assets, build the widget tree here.
}

void UI_Task::onTouch(const TouchEvent &ev)
{
    // Route to the widget under (ev.x, ev.y): Down = press, Move = drag, Up = release.
    (void)ev;
}

void UI_Task::onFrame(uint16_t *fb, uint16_t w, uint16_t h)
{
    // Draw into fb (the back buffer; contents are two frames old, so redraw
    // everything that changed or clear first). Read live data through get():
    //
    //   Value v;
    //   if (_robot.peripheral(Peripheral::Battery)->get(BatteryProp::VoltageMv, v) == SUCCESS)
    //       draw_battery(fb, v.asUInt());
    //
    // PSRAM is slow to read: keep primitives write-only, and blend in a small
    // internal-RAM tile buffer rather than reading the frame buffer back.
    (void)fb;
    (void)w;
    (void)h;
}
