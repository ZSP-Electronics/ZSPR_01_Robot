#pragma once

#include "RGBRoboFace.h"

#if ZGFX_AVAILABLE

// The animated robot face as a zgfx widget. Give it the whole screen: render() clears the
// clip to the face background, so it is meant to fill what it draws into. The
// zgfx::Screen repaints only the parts of the face that moved.
//
// Call tick() once per UI frame (UI_Base::refresh()). It advances the animation and marks
// the changed areas dirty; draw() is then called by the Screen for those areas only.
// Configure the face through face() (eye size, colours, autoblink, expression, ...) before
// or after adding it to a screen.
class RoboFaceWidget : public zgfx::Widget
{
public:
    explicit RoboFaceWidget(const zgfx::Rect &area) : zgfx::Widget(area), _face(area.w, area.h)
    {
        _face.begin(area.w, area.h);
    }

    RGBroboFace &face() { return _face; }

    void tick()
    {
        _face.tick();
        zgfx::Rect dirty[Animated_Face::MAX_DIRTY];
        uint8_t n = _face.takeDirty(dirty);
        for (uint8_t i = 0; i < n; i++)
        {
            // The face works in its own 0,0-based space; the widget may sit elsewhere.
            dirty[i].x += _bounds.x;
            dirty[i].y += _bounds.y;
            invalidate(dirty[i]);
        }
    }

    void draw(zgfx::Canvas &c) override { _face.render(c, _bounds.x, _bounds.y); }
    // render() clears its clip to the face background first, so every pixel is painted.
    bool opaque() const override { return true; }

private:
    RGBroboFace _face;
};

#endif
