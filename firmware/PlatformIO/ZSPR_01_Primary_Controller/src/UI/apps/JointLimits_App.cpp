#include "JointLimits_App.h"
#include <stdio.h>

void JointLimits_App::onStart()
{
    _screen.add(&_title);
    _screen.add(&_status);

    // Created here, in the UI task, like the other apps' dynamic widgets.
    for (uint8_t i = 0; i < COUNT; i++)
    {
        Row &r = _rows[i];
        const int y = ROW_Y0 + i * ROW_PITCH;

        r.name = new zgfx::Label(zgfx::Rect{NAME_X, (int16_t)(y + 5), 228, 16}, SERVO_SLOT_NAME[FIRST + i],
                                 zgfx::colors::LightGray, zgfx::colors::Black, 2);
        _screen.add(r.name);

        r.minCell = {this, i, false};
        r.maxCell = {this, i, true};
        r.minBtn = new zgfx::Button(zgfx::Rect{MIN_X, (int16_t)y, BTN_W, 26}, "Min");
        r.maxBtn = new zgfx::Button(zgfx::Rect{MAX_X, (int16_t)y, BTN_W, 26}, "Max");
        r.minBtn->setOnClick(&JointLimits_App::s_clicked, &r.minCell);
        r.maxBtn->setOnClick(&JointLimits_App::s_clicked, &r.maxCell);
        _screen.add(r.minBtn);
        _screen.add(r.maxBtn);
    }
}

// Torque off so every joint moves freely; rows for servos that can not be captured are
// greyed out.
void JointLimits_App::onEnter()
{
    _active = _procedure.limitCaptureBegin();
    for (uint8_t i = 0; i < COUNT; i++)
    {
        const bool usable = _active && _procedure.calibratable(FIRST + i);
        setButtonLook(_rows[i].minBtn, usable, false);
        setButtonLook(_rows[i].maxBtn, usable, false);
    }
    if (_active)
        setMessage("Move joints, tap Min / Max", zgfx::colors::LightGray);
    else
        setMessage("Servos unavailable", zgfx::colors::Red);
}

// Leaving the app (Home): store the limits, torque back on.
void JointLimits_App::onExit()
{
    if (_active)
        _procedure.limitCaptureEnd(true);
    _active = false;
    setMessage("", zgfx::colors::LightGray);
}

// 64x64 launcher icon: a travel range with an end stop at each side and a knob between.
void JointLimits_App::drawIcon(zgfx::Canvas &c)
{
    using namespace zgfx;
    c.fillRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, rgb(30, 90, 80));
    c.drawRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, colors::LightGray);
    c.drawHLine(14, 32, 36, colors::LightGray);
    c.fillRect(Rect{12, 20, 3, 25}, colors::Yellow);
    c.fillRect(Rect{49, 20, 3, 25}, colors::Yellow);
    c.fillCircle(28, 32, 5, colors::White);
}

void JointLimits_App::capture(uint8_t index, bool isMax)
{
    if (!_active || index >= COUNT)
        return;
    int16_t pos = 0;
    if (_procedure.limitCapture(FIRST + index, isMax, &pos))
    {
        setButtonLook(isMax ? _rows[index].maxBtn : _rows[index].minBtn, true, true);
        char text[zgfx::Label::MAX_TEXT];
        snprintf(text, sizeof(text), "%s %s %d", SERVO_SLOT_NAME[FIRST + index], isMax ? "max" : "min", pos);
        setMessage(text, zgfx::colors::Green);
    }
    else
    {
        setMessage("No answer from servo", zgfx::colors::Red);
    }
}

void JointLimits_App::s_clicked(void *cell)
{
    const Cell *c = static_cast<const Cell *>(cell);
    c->owner->capture(c->index, c->isMax);
}

// Normal blue, green once captured this visit, grey when the servo can not be captured.
void JointLimits_App::setButtonLook(zgfx::Button *b, bool enabled, bool captured)
{
    using namespace zgfx;
    if (!enabled)
        b->setColors(colors::Gray, colors::DarkGray, colors::DarkGray, colors::Gray);
    else if (captured)
        b->setColors(colors::White, rgb(30, 110, 60), rgb(50, 150, 90), colors::LightGray);
    else
        b->setColors(colors::White, rgb(40, 60, 110), rgb(80, 110, 190), colors::LightGray);
}

void JointLimits_App::setMessage(const char *text, zgfx::color_t color)
{
    _status.setColors(color, zgfx::colors::Black);
    _status.setText(text);
}
