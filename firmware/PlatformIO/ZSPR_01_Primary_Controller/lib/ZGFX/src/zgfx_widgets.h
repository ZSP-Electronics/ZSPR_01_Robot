#pragma once

#include "zgfx_canvas.h"
#include "zgfx_dirty.h"

namespace zgfx
{
    class Screen;

    // A rectangular thing that draws itself and may take touch input.
    //
    // draw() works in screen coordinates and is called with the canvas clipped to the
    // area being redrawn, so a widget just draws itself in full and the clip trims it.
    // Anything that changes what draw() would produce must call invalidate().
    class Widget
    {
    public:
        virtual ~Widget() {}

        const Rect &bounds() const { return _bounds; }
        void setBounds(const Rect &r); // redraws the old and the new area

        bool visible() const { return _visible; }
        void setVisible(bool v);

        // Redraw this widget's area on the next frame. No-op until added to a Screen.
        void invalidate();
        // Only part of the widget changed: redraw just r (clipped to the bounds).
        // draw() is still called for the whole widget, but the clip trims it to r.
        void invalidate(const Rect &r);

        virtual void draw(Canvas &c) = 0;

        // True if draw() paints every pixel of the bounds. The Screen then skips its
        // background clear, and the widgets below, for areas this one fully covers:
        // fewer frame-buffer writes, which matters when the buffers sit in PSRAM.
        virtual bool opaque() const { return false; }

        // Touch in screen coordinates. Down goes to the topmost widget under the
        // finger; Move and Up then keep going to that widget even if the finger
        // leaves its bounds. Return true if handled.
        virtual bool onTouch(const TouchEvent &e)
        {
            (void)e;
            return false;
        }

    protected:
        Widget() : _bounds{0, 0, 0, 0} {}
        explicit Widget(const Rect &r) : _bounds(r) {}
        Rect _bounds;

    private:
        friend class Screen;
        Screen *_screen = nullptr;
        bool _visible = true;
    };

    // ---------------------------------------------------------------------

    class Label : public Widget
    {
    public:
        static constexpr size_t MAX_TEXT = 48;

        Label(const Rect &r, const char *text, color_t fg = colors::White, color_t bg = colors::Black, int scale = 1);

        void setText(const char *s); // redraws only if the text changed
        const char *text() const { return _text; }
        void setColors(color_t fg, color_t bg);

        void draw(Canvas &c) override;

    private:
        char _text[MAX_TEXT];
        color_t _fg;
        color_t _bg;
        int _scale;
    };

    class Button : public Widget
    {
    public:
        static constexpr size_t MAX_TEXT = 32;
        using Callback = void (*)(void *user);

        Button(const Rect &r, const char *label);

        void setLabel(const char *s);
        void setOnClick(Callback cb, void *user)
        {
            _cb = cb;
            _user = user;
        }
        void setColors(color_t fg, color_t face, color_t facePressed, color_t border);
        void setScale(int scale);
        bool pressed() const { return _pressed; }

        void draw(Canvas &c) override;
        bool onTouch(const TouchEvent &e) override;

    private:
        char _text[MAX_TEXT];
        color_t _fg = colors::White;
        color_t _face = rgb(40, 60, 110);
        color_t _facePressed = rgb(80, 110, 190);
        color_t _border = colors::LightGray;
        int _scale = 2;
        bool _pressed = false;
        Callback _cb = nullptr;
        void *_user = nullptr;
    };

    class ProgressBar : public Widget
    {
    public:
        explicit ProgressBar(const Rect &r, int32_t minValue = 0, int32_t maxValue = 100);

        void setValue(int32_t v); // clamped to the range
        int32_t value() const { return _value; }
        void setColors(color_t fill, color_t back, color_t border);

        void draw(Canvas &c) override;

    private:
        int32_t _min;
        int32_t _max;
        int32_t _value;
        color_t _fill = colors::Green;
        color_t _back = colors::Black;
        color_t _border = colors::LightGray;
    };

    // Round compass: fixed north-up dial with N/E/S/W and a needle at the heading.
    // The dial is the largest circle that fits in the bounds.
    class Compass : public Widget
    {
    public:
        explicit Compass(const Rect &r) : Widget(r) {}

        // Degrees, 0 = north, clockwise. Redraws only after it has moved
        // MIN_STEP degrees, so sensor jitter doesn't repaint every frame.
        void setHeading(float degrees);
        void setValid(bool valid); // false: needle greyed out
        float heading() const { return _heading; }

        void draw(Canvas &c) override;

    private:
        static constexpr float MIN_STEP = 1.0f;
        Rect needleRect(float heading) const; // where the needle at `heading` draws
        float _heading = 0;
        bool _valid = false;
        bool _seen = false;
    };

    // Round attitude indicator: sky above, ground below, tilted by roll and
    // shifted by pitch, with a fixed aircraft marker in the middle.
    // Positive pitch = nose up (horizon moves down); positive roll = right side
    // down (horizon tilts up on the right).
    class Horizon : public Widget
    {
    public:
        explicit Horizon(const Rect &r) : Widget(r) {}

        // Redraws only after pitch or roll has moved MIN_STEP degrees.
        void setAttitude(float pitchDeg, float rollDeg);
        void setValid(bool valid); // false: shows a level horizon, greyed
        float pitch() const { return _pitch; }
        float roll() const { return _roll; }

        void draw(Canvas &c) override;

    private:
        static constexpr float MIN_STEP = 1.0f;
        float _pitch = 0;
        float _roll = 0;
        bool _valid = false;
        bool _seen = false;
    };

    // ---------------------------------------------------------------------

    // Owns nothing: holds pointers to widgets the application keeps alive. Widgets
    // are drawn in the order they were added (last = on top) and hit-tested in
    // reverse. Fixed capacity, no heap.
    class Screen
    {
    public:
        static constexpr uint8_t MAX_WIDGETS = 32;
        static constexpr uint8_t MAX_TOUCHES = 5;

        Screen(int16_t width, int16_t height, color_t background = colors::Black);

        bool add(Widget *w); // false if full or already added
        bool remove(Widget *w);

        void setBackground(color_t c);
        void invalidateAll() { _dirty.addAll(); }
        DirtyTracker &dirty() { return _dirty; }

        // Feed a touch event (screen coordinates).
        void touch(const TouchEvent &e);

        // Redraw into `target` (the buffer about to be shown) every area that
        // changed this frame or last. Returns true if anything was drawn, i.e. the
        // buffer should be presented.
        bool render(Canvas &target);

        // Call after presenting.
        void endFrame() { _dirty.nextFrame(); }

    private:
        struct Capture
        {
            Widget *w = nullptr;
            uint8_t id = 0;
        };

        Widget *_widgets[MAX_WIDGETS] = {};
        uint8_t _count = 0;
        Capture _captures[MAX_TOUCHES];
        DirtyTracker _dirty;
        color_t _background;
        int16_t _w;
        int16_t _h;
    };
}
