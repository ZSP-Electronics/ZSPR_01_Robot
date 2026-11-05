#pragma once

#include "../UI_Base.h"
#include "UI_IconButton.h"

// A panel of app icons that floats over whatever else is on a screen.
//
// Closed, it is invisible and takes no touches (the widgets under it work
// normally). Open, it draws a panel with one icon tile per app, placed at each
// app's iconPos(), and tapping a tile calls the select callback. Tapping the
// screen outside the panel closes it.
//
// The overlay does not detect the gesture that opens it: the owning screen calls
// open()/close()/toggle(), typically from UI_Base::onSwipe().
//
// attach() adds the overlay and its tiles to a zgfx::Screen. Screen draws in add
// order and hit-tests in reverse, so attach the overlay LAST: it then draws over,
// and gets touches before, everything else on that screen.
class UI_Overlay : public zgfx::Widget
{
public:
    using SelectCallback = void (*)(void *user, UI_Base *app);
    static constexpr uint8_t MAX_APPS = 16;

    // area: the whole screen (so a tap outside the panel can be caught).
    // panel: the visible rounded panel the tiles sit in.
    UI_Overlay(const zgfx::Rect &area, const zgfx::Rect &panel, const char *title, SelectCallback cb, void *user)
        : zgfx::Widget(area), _panel(panel), _title(title), _cb(cb), _user(user)
    {
    }

    // Before attach().
    bool addApp(UI_Base *app)
    {
        if (!app || _attached || _count >= MAX_APPS)
            return false;
        _entries[_count].owner = this;
        _entries[_count++].app = app;
        return true;
    }
    uint8_t appCount() const { return _count; }
    UI_Base *app(uint8_t i) const { return i < _count ? _entries[i].app : nullptr; }

    // Once, from the UI task, after every app can render its icon. Starts closed.
    void attach(zgfx::Screen &screen)
    {
        if (_attached)
            return;
        _attached = true;
        screen.add(this);
        for (uint8_t i = 0; i < _count; i++)
        {
            Entry &e = _entries[i];
            e.tile = new UI_IconButton(e.app->iconPos(), e.app->icon(), e.app->name(), &UI_Overlay::s_tapped, &e);
            screen.add(e.tile);
        }
        applyVisibility();
    }

    bool isOpen() const { return _open; }
    void open() { setOpen(true); }
    void close() { setOpen(false); }
    void toggle() { setOpen(!_open); }

    void draw(zgfx::Canvas &c) override
    {
        using namespace zgfx;
        // Only the panel is painted; the rest of the area is there just to catch touches.
        c.fillRoundRect(_panel, 18, rgb(18, 24, 44));
        c.drawRoundRect(_panel, 18, colors::LightGray);
        const int scale = 2;
        c.drawText(_panel.x + (_panel.w - Canvas::textWidth(_title, scale)) / 2, _panel.y + 12, _title, colors::LightGray, scale);
    }

    bool onTouch(const zgfx::TouchEvent &e) override
    {
        switch (e.type)
        {
        case zgfx::TouchEvent::Down:
            _downOutside = !_panel.contains(e.x, e.y);
            return true;
        case zgfx::TouchEvent::Move:
            return true;
        case zgfx::TouchEvent::Up:
            if (_downOutside && !_panel.contains(e.x, e.y))
                close();
            _downOutside = false;
            return true;
        }
        return false;
    }

private:
    struct Entry
    {
        UI_Overlay *owner = nullptr;
        UI_Base *app = nullptr;
        UI_IconButton *tile = nullptr;
    };
    Entry _entries[MAX_APPS];
    uint8_t _count = 0;
    zgfx::Rect _panel;
    const char *_title;
    SelectCallback _cb;
    void *_user;
    bool _attached = false;
    bool _open = false;
    bool _downOutside = false;

    void setOpen(bool open)
    {
        if (open == _open || !_attached)
            return;
        _open = open;
        applyVisibility();
    }

    void applyVisibility()
    {
        setVisible(_open);
        for (uint8_t i = 0; i < _count; i++)
            if (_entries[i].tile)
                _entries[i].tile->setVisible(_open);
    }

    static void s_tapped(void *entry)
    {
        Entry *e = static_cast<Entry *>(entry);
        UI_Overlay *self = e->owner;
        self->close();
        if (self->_cb)
            self->_cb(self->_user, e->app);
    }
};
