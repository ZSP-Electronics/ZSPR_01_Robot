#include "Dashboard_App.h"
#include <stdio.h>
#include <math.h>

void Dashboard_App::onStart()
{
    // Draw order is add order: later widgets are on top, and get touches first.
    _screen.add(&_title);
    _screen.add(&_battLabel);
    _screen.add(&_battValue);
    _screen.add(&_battBar);
    _screen.add(&_currLabel);
    _screen.add(&_currValue);
    _screen.add(&_horizon);
    _screen.add(&_compass);
    _screen.add(&_imuValue);
    _screen.add(&_modeLabel);
    _screen.add(&_bottangoBtn);

    _bottangoBtn.setOnClick(&Dashboard_App::s_bottangoClicked, this);
    _bottangoBtn.setScale(3);
    _battBar.setColors(zgfx::colors::Green, zgfx::colors::Black, zgfx::colors::LightGray);

    // Without Bottango compiled in there is nothing for the button to do.
    if (!_robot.bottango || !_robot.bottango->enabled())
        _bottangoBtn.setVisible(false);
}

// 64x64 launcher icon: a small dial with a needle.
void Dashboard_App::drawIcon(zgfx::Canvas &c)
{
    using namespace zgfx;
    c.fillRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, rgb(20, 70, 50));
    c.drawRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, colors::LightGray);
    c.drawCircle(32, 32, 20, colors::White);
    c.drawLine(32, 32, 44, 20, colors::Green);
    c.fillCircle(32, 32, 3, colors::White);
}

// Orientation and heading: roll/pitch from the accelerometer, heading tilt-compensated.
void Dashboard_App::refreshImu()
{
    char text[zgfx::Label::MAX_TEXT];
    Value v;
    bool attOk = false, hdgOk = false;
    float pitch = 0, roll = 0, hdg = 0;

    Universal_Module *imu = _robot.peripheral(Peripheral::IMU);
    if (imu)
    {
        // One lock around the reads: a single wait for the IMU task, all values
        // from the same update(). IMU get() doesn't touch the I2C bus (it returns
        // cached values), so this never waits on other I2C users.
        Universal_Module::DataLock lock(*imu);
        attOk = imu->get(ImuProp::AccelOk, v) == SUCCESS && v.asBool();
        if (attOk)
        {
            attOk = imu->get(ImuProp::Pitch, v) == SUCCESS;
            pitch = UI_PITCH_SIGN * v.asFloat();
            attOk = attOk && imu->get(ImuProp::Roll, v) == SUCCESS;
            roll = UI_ROLL_SIGN * v.asFloat();
        }
        hdgOk = imu->get(ImuProp::HeadingOk, v) == SUCCESS && v.asBool();
        if (hdgOk)
        {
            hdgOk = imu->get(ImuProp::Heading, v) == SUCCESS;
            hdg = v.asFloat() + UI_HEADING_OFFSET_DEG;
        }
    }

    // Light low-pass so sensor noise doesn't make the dials shimmer (and repaint
    // every frame); the first sample, or one after a dropout, is taken as is.
    if (attOk)
    {
        _fPitch = _fAttSeen ? _fPitch + UI_IMU_SMOOTHING * (pitch - _fPitch) : pitch;
        _fRoll = _fAttSeen ? _fRoll + UI_IMU_SMOOTHING * (roll - _fRoll) : roll;
        _fAttSeen = true;
    }
    else
    {
        _fAttSeen = false;
    }
    if (hdgOk)
    {
        if (_fHdgSeen)
        {
            float d = fmodf(hdg - _fHdg, 360.0f); // shortest way round the circle
            if (d > 180.0f)
                d -= 360.0f;
            else if (d < -180.0f)
                d += 360.0f;
            _fHdg += UI_IMU_SMOOTHING * d;
        }
        else
        {
            _fHdg = hdg;
        }
        _fHdgSeen = true;
    }
    else
    {
        _fHdgSeen = false;
    }

    _horizon.setValid(attOk);
    _compass.setValid(hdgOk);
    if (attOk)
        _horizon.setAttitude(_fPitch, _fRoll);
    if (hdgOk)
        _compass.setHeading(_fHdg);

    // Text readout from the dials' own (stepped) values, so it only repaints
    // when a dial does.
    if (attOk && hdgOk)
        snprintf(text, sizeof(text), "H%03ld P%+03ld R%+03ld", lroundf(_compass.heading()),
                 lroundf(_horizon.pitch()), lroundf(_horizon.roll()));
    else
        snprintf(text, sizeof(text), "IMU ---");
    _imuValue.setText(text);
}

// Reads live values into the widgets. Setters only mark an area dirty when the
// value actually changed, so calling this every frame costs almost nothing and
// redraws only what moved.
void Dashboard_App::refresh()
{
    char text[zgfx::Label::MAX_TEXT];
    Value v;

    // Battery voltage (from the battery controller board over Serial2)
    Universal_Module *battery = _robot.peripheral(Peripheral::Battery);
    if (battery && battery->get(BatteryProp::VoltageMv, v) == SUCCESS && v.asUInt() > 0)
    {
        uint32_t mv = v.asUInt();
        snprintf(text, sizeof(text), "%lu.%02lu V", (unsigned long)(mv / 1000), (unsigned long)((mv % 1000) / 10));
        _battValue.setText(text);
        _battBar.setValue((int32_t)mv);
    }
    else
    {
        _battValue.setText("---");
        _battBar.setValue(UI_BATT_MIN_MV);
    }

    // System current (INA232, cached by Current_Module's update)
    Universal_Module *current = _robot.peripheral(Peripheral::Current);
    if (current && current->get(CurrentProp::SysCurrent_mA, v) == SUCCESS)
    {
        snprintf(text, sizeof(text), "%ld mA", (long)v.asInt());
        _currValue.setText(text);
    }
    else
    {
        _currValue.setText("---");
    }

    refreshImu();

    // Serial port owner
    bool bottango = _robot.mode() == ControlMode::Bottango;
    _modeLabel.setText(bottango ? "Mode: Bottango (CLI off)" : "Mode: CLI");
    _bottangoBtn.setLabel(bottango ? "Stop Bottango" : "Bottango");
}

// ---------------------------------------------------------------------------
// Bottango mode switch
// ---------------------------------------------------------------------------

void Dashboard_App::s_bottangoClicked(void *self)
{
    static_cast<Dashboard_App *>(self)->toggleBottango();
}

// Runs in the UI task (from Screen::touch). Leaving Bottango mode waits up to a
// second for the Bottango task to stop; the watchdog is fed inside set_mode().
void Dashboard_App::toggleBottango()
{
    _robot.set_mode(_robot.mode() == ControlMode::Cli ? ControlMode::Bottango : ControlMode::Cli);
}
