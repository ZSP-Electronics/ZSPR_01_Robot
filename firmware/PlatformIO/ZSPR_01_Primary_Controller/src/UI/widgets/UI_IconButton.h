#pragma once

#include "zGFX.h"

// A launcher tile: 64x64 icon with the app name centred underneath. Tapping it
// (press and release on the tile) calls the callback.
class UI_IconButton : public zgfx::Widget
{
public:
    using Callback = void (*)(void *user);
    static constexpr int ICON = 64;
    static constexpr int NAME_SCALE = 2;
    static constexpr int PAD = 4;

    // iconPos is the icon's top-left; the tile extends around it to fit the name.
    UI_IconButton(zgfx::Point iconPos, const uint16_t *icon, const char *name, Callback cb, void *user)
        : zgfx::Widget(boundsFor(iconPos, name)), _pos(iconPos), _icon(icon), _name(name), _cb(cb), _user(user)
    {
    }

    void draw(zgfx::Canvas &c) override
    {
        if (_pressed)
            c.fillRoundRect(_bounds, 8, zgfx::rgb(40, 60, 110));
        if (_icon)
            c.blitPixels(_icon, ICON, ICON, _pos);
        int tw = zgfx::Canvas::textWidth(_name, NAME_SCALE);
        c.drawText(_pos.x + (ICON - tw) / 2, _pos.y + ICON + PAD, _name, zgfx::colors::White, NAME_SCALE);
    }

    bool onTouch(const zgfx::TouchEvent &e) override
    {
        switch (e.type)
        {
        case zgfx::TouchEvent::Down:
            setPressed(true);
            return true;
        case zgfx::TouchEvent::Move:
            setPressed(_bounds.contains(e.x, e.y));
            return true;
        case zgfx::TouchEvent::Up:
        {
            bool fire = _pressed && _bounds.contains(e.x, e.y);
            setPressed(false);
            if (fire && _cb)
                _cb(_user);
            return true;
        }
        }
        return false;
    }

private:
    zgfx::Point _pos;
    const uint16_t *_icon;
    const char *_name;
    Callback _cb;
    void *_user;
    bool _pressed = false;

    void setPressed(bool p)
    {
        if (p != _pressed)
        {
            _pressed = p;
            invalidate();
        }
    }

    static zgfx::Rect boundsFor(zgfx::Point p, const char *name)
    {
        int w = zgfx::imax(ICON, zgfx::Canvas::textWidth(name, NAME_SCALE)) + 2 * PAD;
        int h = ICON + 3 * PAD + zgfx::Canvas::textHeight(name, NAME_SCALE);
        return zgfx::Rect{(int16_t)(p.x + (ICON - w) / 2), (int16_t)(p.y - PAD), (int16_t)w, (int16_t)h};
    }
};
