#pragma once

#include "zgfx_types.h"
#include "zgfx_font.h"

namespace zgfx
{
    // A non-owning view of a pixel buffer. Point it at a frame buffer, a widget's
    // cached sprite, or a small tile in internal RAM -- the drawing code is the same.
    //
    // Coordinates are canvas pixels with (0,0) at the top-left. Every call is clipped
    // to the canvas bounds and to the clip rect, so callers never bounds-check.
    //
    // Writes are cheap, reads are not (the frame buffer lives in PSRAM): nothing here
    // reads the destination except getPixel() and the keyed blit's source.
    class Canvas
    {
    public:
        Canvas() = default;

        // stride is the distance between rows in pixels; 0 means width (tightly packed).
        Canvas(color_t *pixels, int16_t width, int16_t height, int32_t stride = 0)
            : _px(pixels), _w(width), _h(height), _stride(stride ? stride : width), _clip{0, 0, width, height}
        {
        }

        bool valid() const { return _px && _w > 0 && _h > 0; }
        int16_t width() const { return _w; }
        int16_t height() const { return _h; }
        int32_t stride() const { return _stride; }
        color_t *pixels() { return _px; }
        const color_t *pixels() const { return _px; }
        Rect bounds() const { return Rect{0, 0, _w, _h}; }

        // ---- clipping ----------------------------------------------------
        void setClip(const Rect &r) { _clip = r.intersect(bounds()); }
        void resetClip() { _clip = bounds(); }
        const Rect &clip() const { return _clip; }

        // A view of part of this canvas (shares the memory). Its (0,0) is r's top-left.
        Canvas sub(const Rect &r) const;

        // ---- primitives ---------------------------------------------------
        void clear(color_t c) { fillRect(_clip, c); } // fills the clip rect

        void drawPixel(int x, int y, color_t c);
        color_t getPixel(int x, int y) const; // 0 outside the canvas

        void drawHLine(int x, int y, int w, color_t c);
        void drawVLine(int x, int y, int h, color_t c);
        void drawLine(int x0, int y0, int x1, int y1, color_t c);

        void drawRect(const Rect &r, color_t c);
        void fillRect(const Rect &r, color_t c);

        // radius is clamped to half the shorter side
        void drawRoundRect(const Rect &r, int radius, color_t c);
        void fillRoundRect(const Rect &r, int radius, color_t c);

        // Solid triangle, any vertex order. Vertices may lie outside the canvas.
        void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, color_t c);

        void drawCircle(int cx, int cy, int radius, color_t c);
        void fillCircle(int cx, int cy, int radius, color_t c);

        // ---- images -------------------------------------------------------
        // Copies `src` (or the srcRect part of it) to `dst`. src's own clip is ignored.
        void blit(const Canvas &src, Point dst);
        void blit(const Canvas &src, const Rect &srcRect, Point dst);
        // Same, but source pixels equal to `key` are skipped (a transparent colour).
        void blitKeyed(const Canvas &src, const Rect &srcRect, Point dst, color_t key);
        // Raw RGB565 pixels, srcStride in pixels (0 = srcW).
        void blitPixels(const color_t *src, int16_t srcW, int16_t srcH, Point dst, int32_t srcStride = 0);

        // ---- text (built-in 5x7 font, integer scale, no anti-aliasing) ----
        // Returns the x just past the last character drawn. '\n' starts a new line at x.
        // Transparent: only the glyph's lit pixels are written.
        int drawText(int x, int y, const char *s, color_t fg, int scale = 1);
        // Opaque: also fills each character cell with bg. Faster and flicker-free for
        // text that changes, because nothing is drawn twice.
        int drawTextOpaque(int x, int y, const char *s, color_t fg, color_t bg, int scale = 1);

        static int textWidth(const char *s, int scale = 1);  // widest line
        static int textHeight(const char *s, int scale = 1); // lines * line height

    private:
        color_t *_px = nullptr;
        int16_t _w = 0;
        int16_t _h = 0;
        int32_t _stride = 0;
        Rect _clip = {0, 0, 0, 0};

        void hspan(int x, int y, int w, color_t c); // already-valid y, clips x
        void blitRaw(const color_t *src, int32_t srcStride, int16_t srcW, int16_t srcH,
                     const Rect &srcRect, Point dst, bool keyed, color_t key);
        int drawTextImpl(int x, int y, const char *s, color_t fg, color_t bg, bool opaque, int scale);
        void drawGlyph(int x, int y, char ch, color_t fg, color_t bg, bool opaque, int scale);
    };
}
