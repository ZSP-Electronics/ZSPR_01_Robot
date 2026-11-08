#pragma once

#include <stdio.h>
#include "../UI_Base.h"
#include "Robot_Procedure.h"

// A status panel that drops over whatever else is on a screen: battery level, system and
// motor rail voltage/current, the battery link and the serial-port mode.
//
// Closed, it is invisible and takes no touches. Open, it draws one panel and swallows
// touches; tapping outside the panel closes it. Like UI_Overlay it does not detect the
// gesture that opens it: the owning screen calls open()/close()/toggle(), typically from
// UI_Base::onSwipe(), and calls refresh() every frame.
//
// refresh() reads the live values only while open, and repaints just the panel, and only
// when a displayed value actually changed.
//
// attach() adds it to a zgfx::Screen. Add it after the widgets it covers.
class UI_Notifications : public zgfx::Widget
{
public:
    // area: the whole screen (so a tap outside the panel can be caught).
    // panel: the visible rounded panel the readouts sit in.
    UI_Notifications(const zgfx::Rect &area, const zgfx::Rect &panel, HAL_Robot_Module &robot, Robot_Procedure &procedure)
        : zgfx::Widget(area), _panel(panel), _robot(robot), _procedure(procedure)
    {
        setVisible(false); // not on a screen yet, so this only clears the flag
    }

    void attach(zgfx::Screen &screen) { screen.add(this); }

    bool isOpen() const { return _open; }
    void open() { setOpen(true); }
    void close() { setOpen(false); }
    void toggle() { setOpen(!_open); }

    // True while the calibration pop-up is up. The owner should not dismiss the panel
    // then (swipes, leaving the screen): servos may be mid-move or have torque off.
    bool isModal() const { return _cal != Cal::Idle; }

    // Every frame, before rendering.
    void refresh()
    {
        if (!_open)
            return;
        stepCalibration();
        Snap s = read();
        if (!same(s, _snap))
        {
            _snap = s;
            invalidate(_panel);
        }
    }

    void draw(zgfx::Canvas &c) override
    {
        using namespace zgfx;
        const int scale = 2;
        c.fillRoundRect(_panel, 18, rgb(18, 24, 44));
        c.drawRoundRect(_panel, 18, colors::LightGray);

        const int left = _panel.x + 14;
        const int right = _panel.x + _panel.w - 14;
        char text[32];

        c.drawText(_panel.x + (_panel.w - Canvas::textWidth("Status", scale)) / 2, _panel.y + 12, "Status", colors::LightGray, scale);

        // Battery: voltage and percentage, then a bar.
        color_t battColor = colors::White;
        int pct = 0;
        if (_snap.battOk)
        {
            pct = percent(_snap.battMv);
            battColor = pct > 30 ? colors::Green : (pct > 15 ? colors::Yellow : colors::Red);
            snprintf(text, sizeof(text), "%lu.%02lu V  %d%%", (unsigned long)(_snap.battMv / 1000),
                     (unsigned long)((_snap.battMv % 1000) / 10), pct);
        }
        else
        {
            snprintf(text, sizeof(text), "---");
        }
        row(c, left, right, _panel.y + 44, "Battery", text, battColor);

        const Rect bar{(int16_t)left, (int16_t)(_panel.y + 66), (int16_t)(right - left), 14};
        c.fillRect(bar, colors::Black);
        c.drawRect(bar, colors::LightGray);
        const int fill = (bar.w - 2) * pct / 100;
        if (fill > 0)
            c.fillRect(Rect{(int16_t)(bar.x + 1), (int16_t)(bar.y + 1), (int16_t)fill, (int16_t)(bar.h - 2)}, battColor);

        // Rails.
        rail(c, left, right, _panel.y + 96, "System", _snap.sysOk, _snap.sysMv, _snap.sysMa);
        rail(c, left, right, _panel.y + 122, "Motor", _snap.motOk, _snap.motMv, _snap.motMa);

        row(c, left, right, _panel.y + 148, "Link", _snap.link ? "OK" : "No link", _snap.link ? colors::Green : colors::Red);
        row(c, left, right, _panel.y + 174, "Mode", _snap.bottango ? "Bottango" : "CLI", colors::White);

        drawButton(c, calibrateRect(), "Calibrate", _snap.canCal);

        if (_cal != Cal::Idle)
            drawDialog(c);
    }

    bool onTouch(const zgfx::TouchEvent &e) override
    {
        switch (e.type)
        {
        case zgfx::TouchEvent::Down:
            _downHit = hit(e.x, e.y);
            _downOutside = !_panel.contains(e.x, e.y);
            return true;
        case zgfx::TouchEvent::Move:
            return true;
        case zgfx::TouchEvent::Up:
        {
            const Hit h = hit(e.x, e.y);
            if (h != Hit::None && h == _downHit)
                activate(h);
            else if (_cal == Cal::Idle && _downOutside && !_panel.contains(e.x, e.y))
                close(); // a pop-up is modal: only its buttons respond
            _downHit = Hit::None;
            _downOutside = false;
            return true;
        }
        }
        return false;
    }

private:
    // Calibration pop-up. The UI only asks and shows; Robot_Procedure runs the calibration
    // in its own task (moving, torque off, reading positions, saving).
    //   Asking: "Calibrate servos?" [Confirm] [Cancel]
    //   Active: follows Robot_Procedure::calibrationState() -- "Calibrating..." while it
    //           works, "Calibration Complete" [Confirm] when the servos are released,
    //           "Calibration failed" [Confirm] if a servo didn't answer.
    enum class Cal : uint8_t { Idle, Asking, Active };
    enum class Hit : uint8_t { None, Calibrate, Confirm, Cancel };
    using ProcState = Robot_Procedure::CalState;

    Cal _cal = Cal::Idle;
    ProcState _shown = ProcState::Idle; // procedure state the pop-up was last drawn for
    Hit _downHit = Hit::None;

    ProcState procState() const { return _procedure.calibrationState(); }
    bool hasConfirm() const
    {
        if (_cal == Cal::Asking)
            return true;
        const ProcState p = procState();
        return _cal == Cal::Active && (p == ProcState::Released || p == ProcState::Failed);
    }

    zgfx::Rect calibrateRect() const
    {
        return zgfx::Rect{(int16_t)(_panel.x + 70), (int16_t)(_panel.y + 204), 200, 34};
    }
    zgfx::Rect dialogRect() const
    {
        return zgfx::Rect{(int16_t)(_panel.x + 20), (int16_t)(_panel.y + 50), 300, 140};
    }
    zgfx::Rect confirmRect() const
    {
        const zgfx::Rect d = dialogRect();
        // Alone and centred on Done/Error; left of Cancel while asking.
        if (_cal == Cal::Asking)
            return zgfx::Rect{(int16_t)(d.x + 20), (int16_t)(d.y + 88), 125, 38};
        return zgfx::Rect{(int16_t)(d.x + 87), (int16_t)(d.y + 88), 125, 38};
    }
    zgfx::Rect cancelRect() const
    {
        const zgfx::Rect d = dialogRect();
        return zgfx::Rect{(int16_t)(d.x + 155), (int16_t)(d.y + 88), 125, 38};
    }

    Hit hit(int x, int y) const
    {
        if (_cal == Cal::Idle)
            return (_snap.canCal && calibrateRect().contains(x, y)) ? Hit::Calibrate : Hit::None;
        if (hasConfirm() && confirmRect().contains(x, y))
            return Hit::Confirm;
        if (_cal == Cal::Asking && cancelRect().contains(x, y))
            return Hit::Cancel;
        return Hit::None;
    }

    void setCal(Cal next)
    {
        _cal = next;
        invalidate(_panel);
    }

    void activate(Hit h)
    {
        Robot_Procedure *proc = &_procedure;
        switch (h)
        {
        case Hit::Calibrate:
            setCal(Cal::Asking);
            break;
        case Hit::Cancel:
            setCal(Cal::Idle);
            break;
        case Hit::Confirm:
            if (_cal == Cal::Asking)
            {
                if (proc->calibrationStart())
                    setCal(Cal::Active);
                else
                    setCal(Cal::Idle);
            }
            else if (procState() == ProcState::Released)
            {
                proc->calibrationConfirm(); // torque on, positions read, offsets saved
            }
            else if (procState() == ProcState::Failed)
            {
                proc->calibrationReset();
                _cal = Cal::Idle;
                close();
            }
            break;
        default:
            break;
        }
    }

    // Follows the calibration task. Done: back to the home screen.
    void stepCalibration()
    {
        if (_cal != Cal::Active)
            return;
        const ProcState p = procState();
        if (p == ProcState::Done)
        {
            _procedure.calibrationReset();
            _cal = Cal::Idle;
            close();
        }
        else if (p == ProcState::Idle) // ended without finishing (Bottango took the bus)
        {
            setCal(Cal::Idle);
        }
        else if (p != _shown)
        {
            _shown = p;
            invalidate(_panel);
        }
    }

    static void drawButton(zgfx::Canvas &c, const zgfx::Rect &r, const char *label, bool enabled)
    {
        using namespace zgfx;
        c.fillRoundRect(r, 8, enabled ? rgb(40, 60, 110) : colors::DarkGray);
        c.drawRoundRect(r, 8, enabled ? colors::LightGray : colors::Gray);
        const int scale = 2;
        c.drawText(r.x + (r.w - Canvas::textWidth(label, scale)) / 2, r.y + (r.h - 14) / 2, label,
                   enabled ? colors::White : colors::Gray, scale);
    }

    void drawDialog(zgfx::Canvas &c) const
    {
        using namespace zgfx;
        const Rect d = dialogRect();
        c.fillRoundRect(d, 14, rgb(30, 36, 64));
        c.drawRoundRect(d, 14, colors::White);

        const char *line1 = "";
        const char *line2 = "";
        color_t color1 = colors::White;
        const ProcState p = _cal == Cal::Active ? procState() : ProcState::Idle;
        if (_cal == Cal::Asking)
        {
            line1 = "Calibrate servos?";
            line2 = "Servos will move";
        }
        else if (p == ProcState::Released)
        {
            line1 = "Calibration Complete";
            line2 = "Set pose, then Confirm";
            color1 = colors::Green;
        }
        else if (p == ProcState::Failed)
        {
            line1 = "Calibration failed";
            line2 = "A servo didn't answer";
            color1 = colors::Red;
        }
        else // Moving / Settling / Releasing / Finishing
        {
            line1 = "Calibrating...";
            line2 = "Please wait";
            color1 = colors::Yellow;
        }
        const int scale = 2;
        c.drawText(d.x + (d.w - Canvas::textWidth(line1, scale)) / 2, d.y + 18, line1, color1, scale);
        c.drawText(d.x + (d.w - Canvas::textWidth(line2, scale)) / 2, d.y + 48, line2, colors::LightGray, scale);

        if (hasConfirm())
            drawButton(c, confirmRect(), "Confirm", true);
        if (_cal == Cal::Asking)
            drawButton(c, cancelRect(), "Cancel", true);
    }

    // Everything draw() shows, so refresh() can tell whether anything changed.
    struct Snap
    {
        bool valid = false;
        bool battOk = false;
        uint32_t battMv = 0;
        bool link = false;
        bool sysOk = false, motOk = false;
        int32_t sysMv = 0, sysMa = 0, motMv = 0, motMa = 0;
        bool bottango = false;
        bool canCal = false; // servo module up and Bottango not holding the bus
    };

    zgfx::Rect _panel;
    HAL_Robot_Module &_robot;
    Robot_Procedure &_procedure;
    Snap _snap;
    bool _open = false;
    bool _downOutside = false;

    void setOpen(bool open)
    {
        if (open == _open)
            return;
        if (!open && _cal != Cal::Idle)
        {
            // Closed mid-calibration (screen left): put torque back, store nothing.
            if (_cal == Cal::Active)
            {
                _procedure.calibrationAbort();
                _procedure.calibrationReset();
            }
            _cal = Cal::Idle;
        }
        _open = open;
        _snap = Snap(); // force the first refresh to repaint with live values
        setVisible(open);
    }

    static bool same(const Snap &a, const Snap &b)
    {
        return a.valid == b.valid && a.battOk == b.battOk && a.battMv == b.battMv && a.link == b.link &&
               a.sysOk == b.sysOk && a.motOk == b.motOk && a.sysMv == b.sysMv && a.sysMa == b.sysMa &&
               a.motMv == b.motMv && a.motMa == b.motMa && a.bottango == b.bottango && a.canCal == b.canCal;
    }

    static int percent(uint32_t mv)
    {
        if (mv <= UI_BATT_MIN_MV)
            return 0;
        if (mv >= UI_BATT_MAX_MV)
            return 100;
        return (int)((mv - UI_BATT_MIN_MV) * 100 / (UI_BATT_MAX_MV - UI_BATT_MIN_MV));
    }

    Snap read()
    {
        Snap s;
        Value v;
        s.valid = true;

        // Same sources the Dashboard uses.
        Universal_Module *battery = _robot.peripheral(Peripheral::Battery);
        if (battery)
        {
            if (battery->get(BatteryProp::VoltageMv, v) == SUCCESS && v.asUInt() > 0)
            {
                s.battOk = true;
                s.battMv = v.asUInt();
            }
            s.link = battery->get(BatteryProp::LinkUp, v) == SUCCESS && v.asBool();
        }

        Universal_Module *current = _robot.peripheral(Peripheral::Current);
        if (current)
        {
            s.sysOk = current->get(CurrentProp::SysVoltage_mV, v) == SUCCESS;
            s.sysMv = v.asInt();
            s.sysOk = s.sysOk && current->get(CurrentProp::SysCurrent_mA, v) == SUCCESS;
            s.sysMa = v.asInt();
            s.motOk = current->get(CurrentProp::MotorVoltage_mV, v) == SUCCESS;
            s.motMv = v.asInt();
            s.motOk = s.motOk && current->get(CurrentProp::MotorCurrent_mA, v) == SUCCESS;
            s.motMa = v.asInt();
        }

        s.bottango = _robot.mode() == ControlMode::Bottango;
        s.canCal = _procedure.calibrationAvailable();
        return s;
    }

    // Label on the left, value flush right.
    static void row(zgfx::Canvas &c, int left, int right, int y, const char *label, const char *value, zgfx::color_t color)
    {
        const int scale = 2;
        c.drawText(left, y, label, zgfx::colors::LightGray, scale);
        c.drawText(right - zgfx::Canvas::textWidth(value, scale), y, value, color, scale);
    }

    static void rail(zgfx::Canvas &c, int left, int right, int y, const char *label, bool ok, int32_t mv, int32_t ma)
    {
        char text[32];
        if (ok)
            snprintf(text, sizeof(text), "%ld.%02ld V %ld mA", (long)(mv / 1000), (long)((mv % 1000) / 10), (long)ma);
        else
            snprintf(text, sizeof(text), "---");
        row(c, left, right, y, label, text, zgfx::colors::White);
    }
};
