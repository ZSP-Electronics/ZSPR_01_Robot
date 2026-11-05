#include "ServoId_App.h"
#include <stdio.h>
#include <string.h>

void ServoId_App::onStart()
{
    // Draw order is add order: later widgets are on top, and get touches first.
    _screen.add(&_title);
    _screen.add(&_scanBtn);
    _screen.add(&_status);
    _screen.add(&_resultLabel);
    _screen.add(&_submitBtn);
    _screen.add(&_newId); // last: its open grid overlaps the Set ID button

    _scanBtn.setScale(3);
    _scanBtn.setOnClick(&ServoId_App::s_scanClicked, this);
    _submitBtn.setScale(3);
    _submitBtn.setOnClick(&ServoId_App::s_submitClicked, this);
}

// Back on the launcher: drop the scan so nothing stale is shown if the bus
// changed in the meantime (servos swapped, Bottango started).
void ServoId_App::onExit()
{
    _newId.setOpen(false);
    _scan = Scan::Idle;
    _foundCount = 0;
    _result[0] = '\0';
}

// 64x64 launcher icon: "ID" with an arrow under it.
void ServoId_App::drawIcon(zgfx::Canvas &c)
{
    using namespace zgfx;
    c.fillRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, rgb(110, 60, 20));
    c.drawRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, colors::LightGray);
    const int scale = 4;
    c.drawText((ICON_SIZE - Canvas::textWidth("ID", scale)) / 2, 10, "ID", colors::White, scale);
    c.drawHLine(14, 50, 32, colors::Yellow);
    c.drawLine(46, 50, 38, 44, colors::Yellow);
    c.drawLine(46, 50, 38, 56, colors::Yellow);
}

bool ServoId_App::busUsable() const
{
    Servo_Module *servo = _robot.servo;
    return servo && servo->enabled() && servo->initialized() && _robot.mode() != ControlMode::Bottango;
}

const char *ServoId_App::typeName(int type)
{
    switch (type)
    {
    case STSERVOTYPE:
        return "ST";
    case SCSERVOTYPE:
        return "SC";
    default:
        return "?";
    }
}

void ServoId_App::startScan()
{
    if (_blocked)
        return;
    _newId.setOpen(false);
    _foundCount = 0;
    _result[0] = '\0';
    _nextId = 0;
    _scan = Scan::Running;
}

// One ping per frame. A silent ID waits out the servo library's timeout, so
// pinging them all at once would freeze the screen for the whole scan.
void ServoId_App::stepScan()
{
    int type = _robot.servo->probe(_nextId);
    if (type != -1 && _foundCount < MAX_SERVO_NUM)
    {
        _foundId[_foundCount] = _nextId;
        _foundType[_foundCount] = type;
        _foundCount++;
    }
    if (++_nextId >= MAX_SERVO_NUM)
        _scan = Scan::Done;
}

// Runs in the UI task (from Screen::touch). changeId() pings and writes on the
// bus, so the screen pauses for a moment (a few hundred ms at worst).
void ServoId_App::submit()
{
    if (!_submitEnabled || _blocked || _scan != Scan::Done || _foundCount != 1)
        return;

    uint8_t oldId = _foundId[0];
    uint8_t newId = (uint8_t)_newId.value();
    _newId.setOpen(false);

    if (newId == oldId)
    {
        snprintf(_result, sizeof(_result), "Servo is already ID %u", oldId);
        return;
    }

    switch (_robot.servo->changeId(oldId, newId))
    {
    case SUCCESS:
        snprintf(_result, sizeof(_result), "ID %u changed to %u", oldId, newId);
        _foundId[0] = newId;
        break;
    case INVALID_PARAM:
        snprintf(_result, sizeof(_result), "Invalid ID");
        break;
    default:
        snprintf(_result, sizeof(_result), "Failed: no reply/ID taken");
        break;
    }
}

void ServoId_App::setSubmitEnabled(bool on)
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

void ServoId_App::updateStatus()
{
    char text[zgfx::Label::MAX_TEXT];
    const char *resultText = _result;

    if (_blocked)
    {
        _status.setText(_robot.mode() == ControlMode::Bottango ? "Bottango is using the bus" : "No servo bus");
        _resultLabel.setText("");
        return;
    }

    switch (_scan)
    {
    case Scan::Idle:
        _status.setText("Press Scan Bus");
        break;
    case Scan::Running:
        snprintf(text, sizeof(text), "Scanning ID %u...", _nextId);
        _status.setText(text);
        break;
    case Scan::Done:
        if (_foundCount == 0)
        {
            _status.setText("No servo detected");
        }
        else if (_foundCount == 1)
        {
            snprintf(text, sizeof(text), "Detected servo ID %u (%s)", _foundId[0], typeName(_foundType[0]));
            _status.setText(text);
        }
        else
        {
            // "N found: 1 4 7", cut off with ".." if it would run past the label.
            int n = snprintf(text, sizeof(text), "%u found:", _foundCount);
            for (uint8_t i = 0; i < _foundCount; i++)
            {
                char id[8];
                int len = snprintf(id, sizeof(id), " %u", _foundId[i]);
                if (n + len > 26)
                {
                    n += snprintf(text + n, sizeof(text) - n, "..");
                    break;
                }
                memcpy(text + n, id, len + 1);
                n += len;
            }
            _status.setText(text);
            if (!resultText[0])
                resultText = "Connect one servo only";
        }
        break;
    }
    _resultLabel.setText(resultText);
}

// Setters only mark an area dirty when the value changed, so this is cheap to
// call every frame.
void ServoId_App::refresh()
{
    _blocked = !busUsable();
    if (_blocked && _scan == Scan::Running)
        _scan = Scan::Idle;

    if (!_blocked && _scan == Scan::Running)
        stepScan();

    _scanBtn.setLabel(_scan == Scan::Running ? "Scanning..." : "Scan Bus");
    setSubmitEnabled(!_blocked && _scan == Scan::Done && _foundCount == 1);
    updateStatus();
}
