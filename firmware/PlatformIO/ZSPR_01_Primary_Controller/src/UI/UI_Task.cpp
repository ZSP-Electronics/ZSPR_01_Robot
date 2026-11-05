#include "UI_Task.h"
#include <esp_task_wdt.h>
#include <cstring>
#include "UI_Apps.h" // after the system headers: the face library defines short global names

bool UI_Task::start()
{
    if (!_robot.display || !_robot.display->initialized())
    {
        return false;
    }

    BaseType_t created = xTaskCreatePinnedToCore(&UI_Task::s_task, "ui", UI_TASK_STACK_BYTES, this,
                                                 UI_TASK_PRIORITY, &_task, UI_TASK_CORE);
    return created == pdPASS;
}

void UI_Task::s_task(void *arg)
{
    static_cast<UI_Task *>(arg)->run();
}

void UI_Task::clearBuffers()
{
    // Both buffers start undefined; clear them so the first present() isn't noise.
    // (Each screen redraws everything on its first two frames anyway.)
    Display_Module *display = _robot.display;
    for (uint8_t i = 0; i < display->frameBufferCount(); i++)
    {
        uint16_t *fb = display->frameBuffer(i);
        if (fb)
            memset(fb, 0, (size_t)_width * _height * sizeof(uint16_t));
    }
}

// Draws the same pattern into every frame buffer and never touches them again, so
// anything that moves, repeats or shifts afterwards comes from scan-out, not drawing.
//   - 40 px grid: repeated or sideways-shifted bands show as broken grid lines
//   - diagonals: a horizontal shift shows as a kink
//   - R/G/B bars along the top: colour order and bit wiring
//   - two 130x160 white rounded rects where the eyes sit
void UI_Task::runTestPattern()
{
    Display_Module *display = _robot.display;
    const int16_t w = (int16_t)_width, h = (int16_t)_height;

    for (uint8_t i = 0; i < display->frameBufferCount(); i++)
    {
        uint16_t *fb = display->frameBuffer(i);
        if (!fb)
            continue;
        zgfx::Canvas c(fb, w, h);
        c.clear(zgfx::colors::Black);
        for (int x = 0; x < w; x += 40)
            c.drawVLine(x, 0, h, zgfx::colors::DarkGray);
        for (int y = 0; y < h; y += 40)
            c.drawHLine(0, y, w, zgfx::colors::DarkGray);
        c.drawLine(0, 0, w - 1, h - 1, zgfx::colors::Yellow);
        c.drawLine(w - 1, 0, 0, h - 1, zgfx::colors::Yellow);
        c.fillRect({120, 80, 80, 30}, zgfx::colors::Red);
        c.fillRect({200, 80, 80, 30}, zgfx::colors::Green);
        c.fillRect({280, 80, 80, 30}, zgfx::colors::Blue);
        c.fillRoundRect({(int16_t)(w / 2 - 20 - 130), (int16_t)(h / 2 - 80), 130, 160}, 40, zgfx::colors::White);
        c.fillRoundRect({(int16_t)(w / 2 + 21), (int16_t)(h / 2 - 80), 130, 160}, 40, zgfx::colors::White);
        c.drawText(170, 380, "TEST", zgfx::colors::Cyan, 5);
    }
    display->present();

    while (1)
    {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Runs in this task. Redraws the new screen in full (two frames, one per buffer).
void UI_Task::switchTo(UI_Base *next)
{
    if (!next || next == _active)
        return;
    if (_active)
        _active->exit();
    _active = next;
    _active->enter();
}

// Overlay icon taps and app Home buttons set a request; act on it between events.
void UI_Task::handleNavigation()
{
    if (UI_Base *app = _active->takeLaunch())
        switchTo(app);
    else if (_active->takeHome())
        switchTo(&_home);
}

void UI_Task::run()
{
    Display_Module *display = _robot.display;

    esp_task_wdt_add(nullptr);

    {
        Value v;
        _width = display->get(DisplayProp::Width, v) == SUCCESS ? (uint16_t)v.asUInt() : 0;
        _height = display->get(DisplayProp::Height, v) == SUCCESS ? (uint16_t)v.asUInt() : 0;
    }

    clearBuffers();

#if UI_TEST_PATTERN
    runTestPattern();
#endif

    // Declared in UI_Apps.h. Apps first: the home screen renders their icons in begin().
    ui_register_apps(_robot, _home);
    for (uint8_t i = 0; i < _home.appCount(); i++)
        _home.app(i)->begin();
    _home.begin();
    switchTo(&_home);

    QueueHandle_t touchQ = display->touchQueue();
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(UI_TASK_PERIOD_MS);

    while (1)
    {
        // Input first, so the frame reflects it.
        TouchEvent ev;
        while (touchQ && xQueueReceive(touchQ, &ev, 0) == pdTRUE)
        {
            _active->touch(ev);
            handleNavigation();
        }

        uint16_t *fb = display->backBuffer();
        if (fb)
        {
            if (_active->frame(fb, _width, _height))
            {
                display->present(); // waits for vsync, so a changing screen paces itself
            }
            _active->endFrame(); // the tracker needs this after every frame, drawn or not
        }

        esp_task_wdt_reset();

        if (xTaskDelayUntil(&lastWake, period) == pdFALSE)
        {
            vTaskDelay(1);
            lastWake = xTaskGetTickCount();
        }
    }
}
