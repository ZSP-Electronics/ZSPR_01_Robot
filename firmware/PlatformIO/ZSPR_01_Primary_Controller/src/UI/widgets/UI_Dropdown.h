#pragma once

#include <stdio.h>
#include "zGFX.h"

// A drop-down that picks one whole number from first..first+count-1.
//
// Closed it is a single box showing "<prefix><value>" and a small arrow. Tapping
// it opens a grid of numbers directly below, same width as the box; tapping a
// number selects it and closes the grid. While open the widget's bounds grow to
// cover the grid, so add it to the screen LAST: it then draws over, and takes
// touches before, whatever sits underneath.
class UI_Dropdown : public zgfx::Widget
{
public:
    static constexpr int MAX_ITEMS = 32;

    // box: the closed box. The grid is `cols` wide and starts at box.bottom().
    UI_Dropdown(const zgfx::Rect &box, const char *prefix, int first, int count, int cols = 4, int cellH = 40)
        : zgfx::Widget(box), _box(box), _prefix(prefix), _first(first),
          _count(count > MAX_ITEMS ? MAX_ITEMS : (count < 1 ? 1 : count)), _cols(cols < 1 ? 1 : cols),
          _cellH(cellH), _value(first)
    {
        int rows = (_count + _cols - 1) / _cols;
        _list = zgfx::Rect{box.x, (int16_t)box.bottom(), box.w, (int16_t)(rows * _cellH)};
    }

    // Called after the grid opens or closes. A screen with several drop-downs uses it to
    // close the others and to move the opened one to the front.
    using ToggleCallback = void (*)(void *user, bool open);
    void setOnToggle(ToggleCallback cb, void *user)
    {
        _toggleCb = cb;
        _toggleUser = user;
    }

    // Puts the grid somewhere other than directly below the box (for a box too close to
    // a screen edge). It keeps the same columns and cell height, so size `r` to
    // rows * cellH. Only takes effect while closed.
    void setListRect(const zgfx::Rect &r)
    {
        if (!_open)
            _list = r;
    }

    int value() const { return _value; }
    void setValue(int v)
    {
        if (v < _first || v >= _first + _count || v == _value)
            return;
        _value = v;
        invalidate();
    }

    bool isOpen() const { return _open; }
    void setOpen(bool open)
    {
        if (open == _open)
            return;
        _open = open;
        _pressed = NONE;
        setBounds(open ? _box.unite(_list) : _box); // redraws the old and the new area
        invalidate();
        if (_toggleCb)
            _toggleCb(_toggleUser, open);
    }

    void draw(zgfx::Canvas &c) override
    {
        using namespace zgfx;

        // Closed box
        c.fillRoundRect(_box, 6, _pressed == HEADER ? rgb(80, 110, 190) : rgb(40, 60, 110));
        c.drawRoundRect(_box, 6, colors::LightGray);
        char text[24];
        snprintf(text, sizeof(text), "%s%d", _prefix, _value);
        const int scale = 3;
        c.drawText(_box.x + 14, _box.y + (_box.h - Canvas::textHeight(text, scale)) / 2, text, colors::White, scale);

        // Arrow on the right edge: down when closed, up when open.
        int ax = _box.x + _box.w - 28;
        int ay = _box.y + _box.h / 2 - 3;
        for (int i = 0; i < 7; i++)
        {
            int w = _open ? 2 * i + 1 : 13 - 2 * i;
            c.drawHLine(ax + 7 - w / 2, ay + i, w, colors::White);
        }

        if (!_open)
            return;

        // Grid
        c.fillRect(_list, colors::Black);
        c.drawRect(_list, colors::LightGray);
        for (int i = 0; i < _count; i++)
        {
            Rect cell = cellRect(i);
            color_t face = rgb(40, 60, 110);
            if (_first + i == _value)
                face = rgb(30, 120, 60);
            if (_pressed == i)
                face = rgb(80, 110, 190);
            c.fillRoundRect(cell, 5, face);
            c.drawRoundRect(cell, 5, colors::Gray);
            snprintf(text, sizeof(text), "%d", _first + i);
            c.drawText(cell.x + (cell.w - Canvas::textWidth(text, scale)) / 2,
                       cell.y + (cell.h - Canvas::textHeight(text, scale)) / 2, text, colors::White, scale);
        }
    }

    bool onTouch(const zgfx::TouchEvent &e) override
    {
        int zone = hit(e.x, e.y);
        switch (e.type)
        {
        case zgfx::TouchEvent::Down:
            _downOutside = _open && zone == NONE; // in the bounds but on neither box nor grid
            setPressed(zone);
            return true;
        case zgfx::TouchEvent::Move:
            if (zone != _pressed)
                setPressed(zone); // pressed only while the finger is over the same spot
            return true;
        case zgfx::TouchEvent::Up:
        {
            bool fire = zone != NONE && zone == _pressed;
            bool dismiss = _downOutside && zone == NONE;
            _downOutside = false;
            setPressed(NONE);
            if (dismiss)
                setOpen(false);
            if (fire)
            {
                if (zone == HEADER)
                    setOpen(!_open);
                else
                {
                    setValue(_first + zone);
                    setOpen(false);
                }
            }
            return true;
        }
        }
        return false;
    }

private:
    static constexpr int NONE = -2;
    static constexpr int HEADER = -1;

    zgfx::Rect _box;
    zgfx::Rect _list;
    const char *_prefix;
    int _first;
    int _count;
    int _cols;
    int _cellH;
    int _value;
    bool _open = false;
    int _pressed = NONE; // HEADER, a cell index, or NONE
    bool _downOutside = false;
    ToggleCallback _toggleCb = nullptr;
    void *_toggleUser = nullptr;

    zgfx::Rect cellRect(int i) const
    {
        int cw = _list.w / _cols;
        int col = i % _cols, row = i / _cols;
        return zgfx::Rect{(int16_t)(_list.x + col * cw + 2), (int16_t)(_list.y + row * _cellH + 2),
                          (int16_t)(cw - 4), (int16_t)(_cellH - 4)};
    }

    int hit(int x, int y) const
    {
        if (_box.contains(x, y))
            return HEADER;
        if (_open && _list.contains(x, y))
        {
            int cw = _list.w / _cols;
            int col = (x - _list.x) / cw;
            int row = (y - _list.y) / _cellH;
            if (col >= _cols)
                return NONE;
            int i = row * _cols + col;
            if (i >= 0 && i < _count)
                return i;
        }
        return NONE;
    }

    void setPressed(int zone)
    {
        if (zone != _pressed)
        {
            _pressed = zone;
            invalidate();
        }
    }
};
