#include "zgfx_dirty.h"

namespace zgfx
{
    void DirtyTracker::List::add(Rect rect)
    {
        if (rect.empty())
            return;

        // Absorb everything it overlaps, so the list never holds overlapping rects
        // (which would be drawn twice).
        bool merged = true;
        while (merged)
        {
            merged = false;
            for (uint8_t i = 0; i < n; i++)
            {
                if (rect.intersects(r[i]))
                {
                    rect = rect.unite(r[i]);
                    r[i] = r[--n];
                    merged = true;
                    break;
                }
            }
        }

        if (n < MAX_RECTS)
        {
            r[n++] = rect;
            return;
        }

        // Full: merge with the rect it grows the least, then re-add the result
        // (it may now overlap others). n shrinks by one first, so this terminates.
        uint8_t best = 0;
        int32_t bestGrowth = 0x7FFFFFFF;
        for (uint8_t i = 0; i < n; i++)
        {
            int32_t growth = rect.unite(r[i]).area() - r[i].area();
            if (growth < bestGrowth)
            {
                bestGrowth = growth;
                best = i;
            }
        }
        Rect u = rect.unite(r[best]);
        r[best] = r[--n];
        add(u);
    }

    void DirtyTracker::add(const Rect &r)
    {
        _cur.add(r.intersect(_bounds));
    }

    uint8_t DirtyTracker::redrawRects(Rect *out, uint8_t cap) const
    {
        List all;
        for (uint8_t i = 0; i < _prev.n; i++)
            all.add(_prev.r[i]);
        for (uint8_t i = 0; i < _cur.n; i++)
            all.add(_cur.r[i]);

        if (!out || cap == 0)
            return all.n;

        uint8_t written = 0;
        for (uint8_t i = 0; i < all.n; i++)
        {
            if (written < cap)
            {
                out[written++] = all.r[i];
            }
            else
            {
                out[cap - 1] = out[cap - 1].unite(all.r[i]); // out of room: fold into the last
            }
        }
        return written;
    }

    void DirtyTracker::nextFrame()
    {
        _prev = _cur;
        _cur.n = 0;
    }
}
