#include "ServoConfig_App.h"
#include <stdio.h>

// The drop-down grid is a 5-wide, 3-row popup in the middle of the screen. It opens in
// the half that doesn't contain the row being edited, so the row stays visible.
static const int POPUP_W = 280;
static const int POPUP_CELL_H = 38;
static const int POPUP_COLS = 5;
static const int POPUP_H = 3 * POPUP_CELL_H; // 13 IDs -> 3 rows of 5
static const int POPUP_X = 100;
static const int POPUP_Y_UPPER = 106; // y 106..220: for rows in the lower half
static const int POPUP_Y_LOWER = 260; // y 260..374: for rows in the upper half

void ServoConfig_App::onStart()
{
    // Draw order is add order: later widgets are on top, and get touches first.
    _screen.add(&_title);
    _screen.add(&_status);
    _screen.add(&_submitBtn);

    _submitBtn.setScale(3);
    _submitBtn.setOnClick(&ServoConfig_App::s_submitClicked, this);

    // Created here, in the UI task, like the other apps' dynamic widgets.
    for (uint8_t i = 0; i < COUNT; i++)
    {
        Row &r = _rows[i];
        r.owner = this;
        r.index = i;
        const int y = ROW_Y0 + i * ROW_PITCH;
        r.name = new zgfx::Label(zgfx::Rect{NAME_X, (int16_t)(y + 5), 228, 16}, SERVO_SLOT_NAME[FIRST + i],
                                 zgfx::colors::LightGray, zgfx::colors::Black, 2);
        _screen.add(r.name);
    }
    // The drop-downs go on last so an open grid draws over, and takes touches before,
    // everything else (rowToggled() also moves the open one to the very top).
    for (uint8_t i = 0; i < COUNT; i++)
    {
        Row &r = _rows[i];
        const int y = ROW_Y0 + i * ROW_PITCH;
        r.drop = new UI_Dropdown(zgfx::Rect{BOX_X, (int16_t)y, 80, 26}, "", ID_FIRST, ID_COUNT, POPUP_COLS, POPUP_CELL_H);
        // Upper popup ends at y 220, so only rows whose box starts below that can use it.
        const int popupY = (y < 220) ? POPUP_Y_LOWER : POPUP_Y_UPPER;
        r.drop->setListRect(zgfx::Rect{POPUP_X, (int16_t)popupY, POPUP_W, POPUP_H});
        r.drop->setOnToggle(&ServoConfig_App::s_toggled, &r);
        _screen.add(r.drop);
    }
}

// Each time the screen opens: show what the module holds now.
void ServoConfig_App::onEnter()
{
    load();
}

void ServoConfig_App::onExit()
{
    for (uint8_t i = 0; i < COUNT; i++)
        _rows[i].drop->setOpen(false);
    setMessage("", zgfx::colors::LightGray);
}

// 64x64 launcher icon: three slider tracks with knobs.
void ServoConfig_App::drawIcon(zgfx::Canvas &c)
{
    using namespace zgfx;
    c.fillRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, rgb(70, 40, 100));
    c.drawRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, colors::LightGray);
    const int knobX[3] = {22, 40, 30};
    for (int i = 0; i < 3; i++)
    {
        const int y = 18 + i * 14;
        c.drawHLine(12, y, 40, colors::LightGray);
        c.fillCircle(knobX[i], y, 4, colors::Yellow);
    }
}

void ServoConfig_App::load()
{
    Servo_Module *servo = _robot.servo;
    _blocked = !(servo && servo->enabled() && servo->initialized());
    setMessage(_blocked ? "No servo module" : "Pick IDs, then Submit",
               _blocked ? zgfx::colors::Red : zgfx::colors::LightGray);
    if (_blocked)
        return;

    for (uint8_t s = 0; s < FIRST; s++)
        _passthroughId[s] = (uint8_t)servo->get_servo_id(s);
    for (uint8_t i = 0; i < COUNT; i++)
    {
        _rows[i].drop->setOpen(false);
        // An ID outside the drop-down's range is ignored, leaving the box as it was.
        _rows[i].drop->setValue(servo->get_servo_id(FIRST + i));
    }
}

// Runs in the UI task (from Screen::touch).
void ServoConfig_App::submit()
{
    if (!_submitEnabled || _blocked)
        return;

    uint8_t ids[MAX_SERVO_NUM];
    for (uint8_t s = 0; s < FIRST; s++)
        ids[s] = _passthroughId[s];
    for (uint8_t i = 0; i < COUNT; i++)
    {
        _rows[i].drop->setOpen(false);
        ids[FIRST + i] = (uint8_t)_rows[i].drop->value();
    }

    // Name the clash rather than just failing: the module refuses duplicates too.
    for (uint8_t a = 0; a < MAX_SERVO_NUM; a++)
    {
        for (uint8_t b = a + 1; b < MAX_SERVO_NUM; b++)
        {
            if (ids[a] == ids[b])
            {
                char text[zgfx::Label::MAX_TEXT];
                snprintf(text, sizeof(text), "ID %u used twice", ids[a]);
                setMessage(text, zgfx::colors::Red);
                return;
            }
        }
    }

    if (_robot.servo->setServoIds(ids, MAX_SERVO_NUM) == SUCCESS)
    {
        if (_robot.servo->saveServoIds())
            setMessage("Saved", zgfx::colors::Green);
        else
            setMessage("Updated, not saved", zgfx::colors::Yellow);
    }
    else
        setMessage("Update failed", zgfx::colors::Red);
}

// One drop-down at a time: opening one closes the rest and brings it above everything,
// since its grid overlaps the other rows' boxes.
void ServoConfig_App::rowToggled(uint8_t index, bool open)
{
    if (!open)
        return;
    for (uint8_t i = 0; i < COUNT; i++)
    {
        if (i != index)
            _rows[i].drop->setOpen(false);
    }
    // Screen draws in add order and hit-tests in reverse; re-adding moves it to the top.
    _screen.remove(_rows[index].drop);
    _screen.add(_rows[index].drop);
}

void ServoConfig_App::s_toggled(void *row, bool open)
{
    Row *r = static_cast<Row *>(row);
    r->owner->rowToggled(r->index, open);
}

void ServoConfig_App::setSubmitEnabled(bool on)
{
    if (on == _submitEnabled)
        return;
    _submitEnabled = on;
    using namespace zgfx;
    if (on)
        _submitBtn.setColors(colors::White, rgb(40, 60, 110), rgb(80, 110, 190), colors::LightGray);
    else
        _submitBtn.setColors(colors::Gray, colors::DarkGray, colors::DarkGray, colors::Gray);
}

void ServoConfig_App::setMessage(const char *text, zgfx::color_t color)
{
    _status.setColors(color, zgfx::colors::Black);
    _status.setText(text);
}

// Setters only mark an area dirty when the value changed, so this is cheap to call
// every frame.
void ServoConfig_App::refresh()
{
    Servo_Module *servo = _robot.servo;
    const bool blocked = !(servo && servo->enabled() && servo->initialized());
    if (blocked && !_blocked)
        setMessage("No servo module", zgfx::colors::Red);
    _blocked = blocked;
    setSubmitEnabled(!_blocked);
}
