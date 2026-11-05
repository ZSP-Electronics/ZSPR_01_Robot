#include "zgfx_widgets.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

namespace zgfx
{
    namespace
    {
        // strncpy that always terminates; returns true if the text changed.
        bool setString(char *dst, size_t cap, const char *src)
        {
            char tmp[64];
            size_t n = src ? strlen(src) : 0;
            if (n >= cap)
                n = cap - 1;
            if (n >= sizeof(tmp))
                n = sizeof(tmp) - 1;
            memcpy(tmp, src ? src : "", n);
            tmp[n] = '\0';
            if (strcmp(dst, tmp) == 0)
                return false;
            memcpy(dst, tmp, n + 1);
            return true;
        }
    }

    // ------------------------------------------------------------------ Widget
    void Widget::setBounds(const Rect &r)
    {
        if (r == _bounds)
            return;
        invalidate(); // where it was
        _bounds = r;
        invalidate(); // where it is now
    }

    void Widget::setVisible(bool v)
    {
        if (v == _visible)
            return;
        _visible = v;
        invalidate();
    }

    void Widget::invalidate()
    {
        if (_screen)
            _screen->dirty().add(_bounds);
    }

    void Widget::invalidate(const Rect &r)
    {
        if (_screen)
            _screen->dirty().add(r.intersect(_bounds));
    }

    // ------------------------------------------------------------------- Label
    Label::Label(const Rect &r, const char *text, color_t fg, color_t bg, int scale)
        : Widget(r), _fg(fg), _bg(bg), _scale(scale < 1 ? 1 : scale)
    {
        _text[0] = '\0';
        setString(_text, MAX_TEXT, text);
    }

    void Label::setText(const char *s)
    {
        if (setString(_text, MAX_TEXT, s))
            invalidate();
    }

    void Label::setColors(color_t fg, color_t bg)
    {
        if (fg == _fg && bg == _bg)
            return;
        _fg = fg;
        _bg = bg;
        invalidate();
    }

    void Label::draw(Canvas &c)
    {
        c.fillRect(_bounds, _bg); // cover the whole box, then the text on top
        c.drawText(_bounds.x, _bounds.y, _text, _fg, _scale);
    }

    // ------------------------------------------------------------------ Button
    Button::Button(const Rect &r, const char *label) : Widget(r)
    {
        _text[0] = '\0';
        setString(_text, MAX_TEXT, label);
    }

    void Button::setLabel(const char *s)
    {
        if (setString(_text, MAX_TEXT, s))
            invalidate();
    }

    void Button::setColors(color_t fg, color_t face, color_t facePressed, color_t border)
    {
        _fg = fg;
        _face = face;
        _facePressed = facePressed;
        _border = border;
        invalidate();
    }

    void Button::setScale(int scale)
    {
        _scale = scale < 1 ? 1 : scale;
        invalidate();
    }

    void Button::draw(Canvas &c)
    {
        c.fillRoundRect(_bounds, 6, _pressed ? _facePressed : _face);
        c.drawRoundRect(_bounds, 6, _border);

        int tw = Canvas::textWidth(_text, _scale);
        int th = Canvas::textHeight(_text, _scale);
        int tx = _bounds.x + (_bounds.w - tw) / 2;
        int ty = _bounds.y + (_bounds.h - th) / 2;
        c.drawText(tx, ty, _text, _fg, _scale);
    }

    bool Button::onTouch(const TouchEvent &e)
    {
        switch (e.type)
        {
        case TouchEvent::Down:
            _pressed = true;
            invalidate();
            return true;
        case TouchEvent::Move:
        {
            // pressed only while the finger is over the button
            bool over = _bounds.contains(e.x, e.y);
            if (over != _pressed)
            {
                _pressed = over;
                invalidate();
            }
            return true;
        }
        case TouchEvent::Up:
        {
            bool fire = _pressed && _bounds.contains(e.x, e.y);
            _pressed = false;
            invalidate();
            if (fire && _cb)
                _cb(_user);
            return true;
        }
        }
        return false;
    }

    // ------------------------------------------------------------- ProgressBar
    ProgressBar::ProgressBar(const Rect &r, int32_t minValue, int32_t maxValue)
        : Widget(r), _min(minValue), _max(maxValue > minValue ? maxValue : minValue + 1), _value(minValue)
    {
    }

    void ProgressBar::setValue(int32_t v)
    {
        v = (int32_t)imax(_min, imin(v, _max));
        if (v == _value)
            return;
        _value = v;
        invalidate();
    }

    void ProgressBar::setColors(color_t fill, color_t back, color_t border)
    {
        _fill = fill;
        _back = back;
        _border = border;
        invalidate();
    }

    void ProgressBar::draw(Canvas &c)
    {
        c.fillRect(_bounds, _back);
        c.drawRect(_bounds, _border);

        int32_t inner = _bounds.w - 2;
        int32_t filled = inner > 0 ? (int32_t)((int64_t)inner * (_value - _min) / (_max - _min)) : 0;
        if (filled > 0 && _bounds.h > 2)
            c.fillRect(Rect{(int16_t)(_bounds.x + 1), (int16_t)(_bounds.y + 1), (int16_t)filled, (int16_t)(_bounds.h - 2)}, _fill);
    }

    // ----------------------------------------------------------------- Compass
    namespace
    {
        const float kDegToRad = 3.14159265f / 180.0f;

        // Point at `radius` from (cx,cy) along compass bearing `deg` (0 = up, clockwise).
        void polar(int cx, int cy, float radius, float deg, int &x, int &y)
        {
            float a = deg * kDegToRad;
            x = cx + (int)lroundf(radius * sinf(a));
            y = cy - (int)lroundf(radius * cosf(a));
        }

        // Smallest absolute difference between two bearings, 0..180.
        float angleDiff(float a, float b)
        {
            float d = fabsf(a - b);
            d = fmodf(d, 360.0f);
            return d > 180.0f ? 360.0f - d : d;
        }

        int dialRadius(const Rect &r)
        {
            return (int)imin(r.w, r.h) / 2 - 1;
        }
    }

    void Compass::setHeading(float degrees)
    {
        degrees = fmodf(degrees, 360.0f);
        if (degrees < 0)
            degrees += 360.0f;
        if (_seen && angleDiff(degrees, _heading) < MIN_STEP)
            return;
        _seen = true;
        // Only the needle moves: repaint where it was and where it goes.
        invalidate(needleRect(_heading));
        _heading = degrees;
        invalidate(needleRect(_heading));
    }

    // Bounding box of the needle (tip, tail, hub and the 1 px width offsets).
    Rect Compass::needleRect(float heading) const
    {
        const int cx = _bounds.x + _bounds.w / 2;
        const int cy = _bounds.y + _bounds.h / 2;
        const float tipLen = (float)(dialRadius(_bounds) - 20);
        int tx, ty, bx, by;
        polar(cx, cy, tipLen, heading, tx, ty);
        polar(cx, cy, tipLen * 0.4f, heading + 180.0f, bx, by);
        const int pad = 4; // hub radius 3 + width offset 1
        int32_t x0 = imin(imin(tx, bx), cx) - pad, x1 = imax(imax(tx, bx), cx) + pad;
        int32_t y0 = imin(imin(ty, by), cy) - pad, y1 = imax(imax(ty, by), cy) + pad;
        return Rect{(int16_t)x0, (int16_t)y0, (int16_t)(x1 - x0 + 1), (int16_t)(y1 - y0 + 1)};
    }

    void Compass::setValid(bool valid)
    {
        if (valid == _valid)
            return;
        _valid = valid;
        invalidate();
    }

    void Compass::draw(Canvas &c)
    {
        const int cx = _bounds.x + _bounds.w / 2;
        const int cy = _bounds.y + _bounds.h / 2;
        const int R = dialRadius(_bounds);
        if (R < 12)
            return;

        c.fillCircle(cx, cy, R, rgb(10, 16, 30));
        c.drawCircle(cx, cy, R, colors::LightGray);

        // Ticks every 30 degrees, longer at the cardinals.
        for (int deg = 0; deg < 360; deg += 30)
        {
            int x0, y0, x1, y1;
            int len = (deg % 90 == 0) ? 6 : 3;
            polar(cx, cy, (float)R, (float)deg, x0, y0);
            polar(cx, cy, (float)(R - len), (float)deg, x1, y1);
            c.drawLine(x0, y0, x1, y1, colors::LightGray);
        }

        // Cardinal letters (5x7 cells, centred on their bearing).
        static const char letters[4] = {'N', 'E', 'S', 'W'};
        for (int i = 0; i < 4; i++)
        {
            int x, y;
            polar(cx, cy, (float)(R - 14), (float)(i * 90), x, y);
            char s[2] = {letters[i], '\0'};
            c.drawText(x - 2, y - 3, s, i == 0 ? colors::Red : colors::White, 1);
        }

        // Needle: three parallel lines for width, then a hub.
        const float tipLen = (float)(R - 20);
        int tx, ty, bx, by;
        polar(cx, cy, tipLen, _heading, tx, ty);
        polar(cx, cy, tipLen * 0.4f, _heading + 180.0f, bx, by);
        const color_t tipColor = _valid ? colors::Red : colors::Gray;
        const color_t tailColor = _valid ? colors::White : colors::DarkGray;
        // Offset across the dominant axis so the width stays ~3 px at any angle.
        bool steep = abs(ty - cy) >= abs(tx - cx);
        for (int o = -1; o <= 1; o++)
        {
            int ox = steep ? o : 0, oy = steep ? 0 : o;
            c.drawLine(cx + ox, cy + oy, tx, ty, tipColor);
            c.drawLine(cx + ox, cy + oy, bx, by, tailColor);
        }
        c.fillCircle(cx, cy, 3, colors::LightGray);
    }

    // ----------------------------------------------------------------- Horizon
    void Horizon::setAttitude(float pitchDeg, float rollDeg)
    {
        if (_seen && fabsf(pitchDeg - _pitch) < MIN_STEP && fabsf(rollDeg - _roll) < MIN_STEP)
            return;
        _seen = true;
        _pitch = pitchDeg;
        _roll = rollDeg;
        invalidate();
    }

    void Horizon::setValid(bool valid)
    {
        if (valid == _valid)
            return;
        _valid = valid;
        invalidate();
    }

    void Horizon::draw(Canvas &c)
    {
        const int cx = _bounds.x + _bounds.w / 2;
        const int cy = _bounds.y + _bounds.h / 2;
        const int R = dialRadius(_bounds);
        if (R < 12)
            return;

        const color_t sky = _valid ? rgb(40, 120, 200) : rgb(50, 60, 75);
        const color_t ground = _valid ? rgb(140, 90, 40) : rgb(70, 65, 60);

        // A pixel at (dx,dy) from the centre is ground when
        //   dx*sin(roll) + dy*cos(roll) > offset
        // where offset slides the horizon down for nose-up pitch (45 deg = full radius).
        float pitch = _valid ? _pitch : 0.0f;
        float roll = (_valid ? _roll : 0.0f) * kDegToRad;
        if (pitch > 60.0f)
            pitch = 60.0f;
        if (pitch < -60.0f)
            pitch = -60.0f;
        const float s = sinf(roll), co = cosf(roll);
        const float offset = pitch * (float)R / 45.0f;

        for (int dy = -R; dy <= R; dy++)
        {
            const int half = (int)sqrtf((float)(R * R - dy * dy));
            const int y = cy + dy;
            const int x0 = cx - half;
            const int w = 2 * half + 1;

            if (fabsf(s) < 1e-4f)
            {
                c.drawHLine(x0, y, w, (dy * co > offset) ? ground : sky);
                continue;
            }

            // Boundary column: dx = (offset - dy*co) / s. Ground is on the
            // large-dx side when s > 0, on the small-dx side when s < 0.
            const float t = (offset - dy * co) / s;
            int split = (int)ceilf(t);                 // first column with dx >= t
            if (split < -half) split = -half;
            if (split > half + 1) split = half + 1;
            const int nLow = split + half;             // columns -half .. split-1
            const int nHigh = w - nLow;                // columns split .. half
            if (s > 0)
            {
                if (nLow > 0)
                    c.drawHLine(x0, y, nLow, sky);
                if (nHigh > 0)
                    c.drawHLine(x0 + nLow, y, nHigh, ground);
            }
            else
            {
                if (nLow > 0)
                    c.drawHLine(x0, y, nLow, ground);
                if (nHigh > 0)
                    c.drawHLine(x0 + nLow, y, nHigh, sky);
            }
        }

        c.drawCircle(cx, cy, R, colors::LightGray);

        // Fixed aircraft marker.
        const int wing = R / 2;
        c.drawHLine(cx - wing, cy, wing / 2, colors::Yellow);
        c.drawHLine(cx + wing - wing / 2 + 1, cy, wing / 2, colors::Yellow);
        c.fillCircle(cx, cy, 2, colors::Yellow);
    }

    // ------------------------------------------------------------------ Screen
    Screen::Screen(int16_t width, int16_t height, color_t background)
        : _dirty(width, height), _background(background), _w(width), _h(height)
    {
        _dirty.addAll(); // nothing has been drawn yet
    }

    bool Screen::add(Widget *w)
    {
        if (!w || _count >= MAX_WIDGETS || w->_screen)
            return false;
        _widgets[_count++] = w;
        w->_screen = this;
        w->invalidate();
        return true;
    }

    bool Screen::remove(Widget *w)
    {
        for (uint8_t i = 0; i < _count; i++)
        {
            if (_widgets[i] != w)
                continue;
            w->invalidate();
            w->_screen = nullptr;
            for (uint8_t j = i; j + 1 < _count; j++)
                _widgets[j] = _widgets[j + 1];
            _widgets[--_count] = nullptr;
            for (Capture &cap : _captures)
            {
                if (cap.w == w)
                    cap.w = nullptr;
            }
            return true;
        }
        return false;
    }

    void Screen::setBackground(color_t c)
    {
        if (c == _background)
            return;
        _background = c;
        invalidateAll();
    }

    void Screen::touch(const TouchEvent &e)
    {
        if (e.type == TouchEvent::Down)
        {
            for (int i = (int)_count - 1; i >= 0; i--)
            {
                Widget *w = _widgets[i];
                if (!w->visible() || !w->bounds().contains(e.x, e.y))
                    continue;
                for (Capture &cap : _captures)
                {
                    if (!cap.w)
                    {
                        cap.w = w;
                        cap.id = e.id;
                        break;
                    }
                }
                w->onTouch(e);
                return;
            }
            return;
        }

        for (Capture &cap : _captures)
        {
            if (cap.w && cap.id == e.id)
            {
                Widget *w = cap.w;
                if (e.type == TouchEvent::Up)
                    cap.w = nullptr;
                w->onTouch(e);
                return;
            }
        }
    }

    bool Screen::render(Canvas &target)
    {
        Rect areas[DirtyTracker::MAX_RECTS];
        uint8_t n = _dirty.redrawRects(areas, DirtyTracker::MAX_RECTS);
        if (n == 0)
            return false;

        for (uint8_t a = 0; a < n; a++)
        {
            target.setClip(areas[a]);

            // Start at the topmost opaque widget covering the whole area: it paints every
            // pixel, so the background clear and anything beneath it would be overdrawn.
            uint8_t first = 0;
            bool covered = false;
            for (uint8_t i = _count; i-- > 0;)
            {
                Widget *w = _widgets[i];
                if (w->visible() && w->opaque() && w->bounds().contains(areas[a]))
                {
                    first = i;
                    covered = true;
                    break;
                }
            }
            if (!covered)
                target.clear(_background);

            for (uint8_t i = first; i < _count; i++)
            {
                Widget *w = _widgets[i];
                if (w->visible() && w->bounds().intersects(areas[a]))
                    w->draw(target);
            }
        }
        target.resetClip();
        return true;
    }
}
