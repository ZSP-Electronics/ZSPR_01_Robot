#pragma once

#include "zgfx_types.h"

namespace zgfx
{
    // Tracks which screen areas changed, for a display that alternates between two
    // frame buffers.
    //
    // The buffer you draw into is two frames behind the one you just built, so a
    // partial redraw is only correct if it covers what changed in THIS frame plus
    // what changed in the PREVIOUS frame (that buffer never saw it). That union is
    // redrawRects().
    //
    // Per frame:
    //     tracker.add(rect) ...            // as things change
    //     n = tracker.redrawRects(out, N)  // draw exactly those areas
    //     present();
    //     tracker.nextFrame();
    //
    // Fixed capacity, no heap. Overlapping rects are merged; if the list is full
    // the new rect is merged into whichever one it grows the least.
    class DirtyTracker
    {
    public:
        static constexpr uint8_t MAX_RECTS = 8;

        DirtyTracker() : _bounds{0, 0, 0, 0} {}
        DirtyTracker(int16_t w, int16_t h) : _bounds{0, 0, w, h} {}

        void setBounds(int16_t w, int16_t h) { _bounds = Rect{0, 0, w, h}; }

        // Mark an area as changed this frame (clipped to the bounds).
        void add(const Rect &r);
        void addAll() { add(_bounds); }

        // Areas to redraw into the buffer about to be drawn: this frame's changes
        // plus the previous frame's, overlaps merged. Returns how many were written
        // to `out`; if cap is smaller than needed the rest are folded into the last.
        uint8_t redrawRects(Rect *out, uint8_t cap) const;

        // Anything to redraw (this frame or last)?
        bool pending() const { return _cur.n || _prev.n; }

        // Call after presenting: this frame's changes become the previous frame's.
        void nextFrame();

        void reset()
        {
            _cur.n = 0;
            _prev.n = 0;
        }

    private:
        struct List
        {
            Rect r[MAX_RECTS];
            uint8_t n = 0;
            void add(Rect rect);
        };

        List _cur;
        List _prev;
        Rect _bounds;
    };
}
