#include "UI_Base.h"
#include <cstring>

void UI_Base::begin()
{
    onStart();
    // Added last so it is drawn on top and gets touches first.
    if (hasHomeButton())
    {
        _homeBtn.setOnClick(&UI_Base::s_homeClicked, this);
        _screen.add(&_homeBtn);
    }
}

void UI_Base::touch(const TouchEvent &ev)
{
    // The display's event and ZGFX's have the same shape; they just live in
    // separate libraries so neither depends on the other.
    zgfx::TouchEvent::Type type = zgfx::TouchEvent::Move;
    switch (ev.type)
    {
    case TouchEvent::Down:
        type = zgfx::TouchEvent::Down;
        break;
    case TouchEvent::Move:
        type = zgfx::TouchEvent::Move;
        break;
    case TouchEvent::Up:
        type = zgfx::TouchEvent::Up;
        break;
    }
    _screen.touch(zgfx::TouchEvent{type, (int16_t)ev.x, (int16_t)ev.y, ev.id});
    detectSwipe(ev);
}

// Runs after the widgets have seen the event, so a swipe that opens something
// (the home screen's app overlay) doesn't change who receives this touch.
void UI_Base::detectSwipe(const TouchEvent &ev)
{
    switch (ev.type)
    {
    case TouchEvent::Down:
        if (!_gesture.active)
        {
            _gesture.active = true;
            _gesture.fired = false;
            _gesture.id = ev.id;
            _gesture.x0 = (int16_t)ev.x;
            _gesture.y0 = (int16_t)ev.y;
        }
        break;
    case TouchEvent::Move:
    {
        if (!_gesture.active || _gesture.fired || ev.id != _gesture.id)
            break;
        int dx = (int)ev.x - _gesture.x0;
        int dy = (int)ev.y - _gesture.y0;
        int ax = dx < 0 ? -dx : dx;
        int ay = dy < 0 ? -dy : dy;
        if (ay >= SWIPE_MIN_PX && ay >= 2 * ax)
        {
            _gesture.fired = true;
            onSwipe(dy < 0 ? Swipe::Up : Swipe::Down);
        }
        else if (ax >= SWIPE_MIN_PX && ax >= 2 * ay)
        {
            _gesture.fired = true;
            onSwipe(dx < 0 ? Swipe::Left : Swipe::Right);
        }
        break;
    }
    case TouchEvent::Up:
        if (_gesture.active && ev.id == _gesture.id)
            _gesture.active = false;
        break;
    }
}

bool UI_Base::frame(uint16_t *fb, uint16_t w, uint16_t h)
{
    refresh();
    // fb is the back buffer: two frames old, which Screen::render() accounts for.
    zgfx::Canvas canvas(fb, (int16_t)w, (int16_t)h);
    return _screen.render(canvas);
}

const uint16_t *UI_Base::icon()
{
    if (!_iconBuf)
    {
        _iconBuf = (uint16_t *)malloc(ICON_SIZE * ICON_SIZE * sizeof(uint16_t));
        if (!_iconBuf)
            return nullptr;
        memset(_iconBuf, 0, ICON_SIZE * ICON_SIZE * sizeof(uint16_t));
        zgfx::Canvas c(_iconBuf, ICON_SIZE, ICON_SIZE);
        drawIcon(c);
    }
    return _iconBuf;
}

void UI_Base::drawIcon(zgfx::Canvas &c)
{
    c.fillRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, zgfx::rgb(40, 60, 110));
    c.drawRoundRect({0, 0, ICON_SIZE, ICON_SIZE}, 12, zgfx::colors::LightGray);
    char s[2] = {_name && _name[0] ? _name[0] : '?', 0};
    const int scale = 5;
    c.drawText((ICON_SIZE - zgfx::Canvas::textWidth(s, scale)) / 2,
               (ICON_SIZE - zgfx::Canvas::textHeight(s, scale)) / 2, s, zgfx::colors::White, scale);
}
