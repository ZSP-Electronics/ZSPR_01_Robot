#include "zgfx_canvas.h"
#include <string.h>

namespace zgfx
{
    // ------------------------------------------------------------------
    // Corner geometry shared by the round-rect routines. A corner of radius r is
    // an r x r square whose circle is centred on (r, r) in pixel-edge coordinates;
    // a pixel (px, py) counts as inside when its centre is within the radius.
    // Doubled coordinates keep it all in integers.
    // ------------------------------------------------------------------
    namespace
    {
        bool cornerInside(int px, int py, int corner, int radius)
        {
            int dx2 = 2 * corner - 2 * px - 1;
            int dy2 = 2 * corner - 2 * py - 1;
            return dx2 * dx2 + dy2 * dy2 <= 4 * radius * radius;
        }

        // How many pixels row `py` of the corner is indented from the rect's edge.
        int cornerInset(int py, int radius)
        {
            int px = 0;
            while (px < radius && !cornerInside(px, py, radius, radius))
                px++;
            return px;
        }

        int clampRadius(int radius, const Rect &r)
        {
            int limit = (int)imin(r.w, r.h) / 2;
            return (int)imax(0, imin(radius, limit));
        }
    }

    Canvas Canvas::sub(const Rect &r) const
    {
        Rect v = r.intersect(bounds());
        if (v.empty() || !_px)
            return Canvas();
        return Canvas(_px + (int32_t)v.y * _stride + v.x, v.w, v.h, _stride);
    }

    // ------------------------------------------------------------------
    // Pixels, lines, rectangles
    // ------------------------------------------------------------------
    void Canvas::drawPixel(int x, int y, color_t c)
    {
        if (_px && _clip.contains(x, y))
            _px[(int32_t)y * _stride + x] = c;
    }

    color_t Canvas::getPixel(int x, int y) const
    {
        if (!_px || !bounds().contains(x, y))
            return 0;
        return _px[(int32_t)y * _stride + x];
    }

    void Canvas::hspan(int x, int y, int w, color_t c)
    {
        int32_t x0 = imax(x, _clip.x);
        int32_t x1 = imin((int32_t)x + w, _clip.right());
        if (x1 <= x0)
            return;
        // A plain loop: the compiler turns it into wide stores. Swap in 32-bit
        // paired stores here if profiling says fills are the bottleneck.
        color_t *p = _px + (int32_t)y * _stride + x0;
        for (int32_t n = x1 - x0; n > 0; n--)
            *p++ = c;
    }

    void Canvas::drawHLine(int x, int y, int w, color_t c)
    {
        if (!_px || w <= 0 || y < _clip.y || y >= _clip.bottom())
            return;
        hspan(x, y, w, c);
    }

    void Canvas::drawVLine(int x, int y, int h, color_t c)
    {
        if (!_px || h <= 0 || x < _clip.x || x >= _clip.right())
            return;
        int32_t y0 = imax(y, _clip.y);
        int32_t y1 = imin((int32_t)y + h, _clip.bottom());
        color_t *p = _px + y0 * _stride + x;
        for (int32_t n = y1 - y0; n > 0; n--, p += _stride)
            *p = c;
    }

    void Canvas::drawLine(int x0, int y0, int x1, int y1, color_t c)
    {
        if (y0 == y1)
        {
            drawHLine(imin(x0, x1), y0, (int)(imax(x0, x1) - imin(x0, x1)) + 1, c);
            return;
        }
        if (x0 == x1)
        {
            drawVLine(x0, imin(y0, y1), (int)(imax(y0, y1) - imin(y0, y1)) + 1, c);
            return;
        }

        int dx = x1 > x0 ? x1 - x0 : x0 - x1;
        int dy = y1 > y0 ? y1 - y0 : y0 - y1;
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;
        for (;;)
        {
            drawPixel(x0, y0, c);
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    void Canvas::fillRect(const Rect &r, color_t c)
    {
        Rect v = r.intersect(_clip);
        if (!_px || v.empty())
            return;
        for (int y = v.y; y < v.bottom(); y++)
            hspan(v.x, y, v.w, c);
    }

    void Canvas::drawRect(const Rect &r, color_t c)
    {
        if (r.empty())
            return;
        drawHLine(r.x, r.y, r.w, c);
        drawHLine(r.x, r.bottom() - 1, r.w, c);
        if (r.h > 2)
        {
            drawVLine(r.x, r.y + 1, r.h - 2, c);
            drawVLine((int)r.right() - 1, r.y + 1, r.h - 2, c);
        }
    }

    // ------------------------------------------------------------------
    // Round rectangles and circles
    // ------------------------------------------------------------------
    void Canvas::fillRoundRect(const Rect &r, int radius, color_t c)
    {
        int rad = clampRadius(radius, r);
        if (rad <= 0)
        {
            fillRect(r, c);
            return;
        }
        for (int i = 0; i < rad; i++)
        {
            int inset = cornerInset(i, rad);
            drawHLine(r.x + inset, r.y + i, r.w - 2 * inset, c);
            drawHLine(r.x + inset, (int)r.bottom() - 1 - i, r.w - 2 * inset, c);
        }
        fillRect(Rect{r.x, (int16_t)(r.y + rad), r.w, (int16_t)(r.h - 2 * rad)}, c);
    }

    void Canvas::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, color_t c)
    {
        if (!_px)
            return;
        // Sort the vertices by y: (x0,y0) top, (x2,y2) bottom.
        if (y0 > y1)
        {
            int t = y0; y0 = y1; y1 = t;
            t = x0; x0 = x1; x1 = t;
        }
        if (y1 > y2)
        {
            int t = y1; y1 = y2; y2 = t;
            t = x1; x1 = x2; x2 = t;
        }
        if (y0 > y1)
        {
            int t = y0; y0 = y1; y1 = t;
            t = x0; x0 = x1; x1 = t;
        }
        if (y0 == y2) // flat: a single line
        {
            int a = (int)imin(x0, imin(x1, x2));
            int b = (int)imax(x0, imax(x1, x2));
            drawHLine(a, y0, b - a + 1, c);
            return;
        }
        int ys = (int)imax(y0, _clip.y);
        int ye = (int)imin(y2, _clip.bottom() - 1);
        for (int y = ys; y <= ye; y++)
        {
            int xa = x0 + (int)((int32_t)(x2 - x0) * (y - y0) / (y2 - y0)); // long edge
            int xb;                                                            // short edges
            if (y < y1)
                xb = x0 + (int)((int32_t)(x1 - x0) * (y - y0) / (y1 - y0));
            else if (y2 > y1)
                xb = x1 + (int)((int32_t)(x2 - x1) * (y - y1) / (y2 - y1));
            else
                xb = x1;
            if (xa > xb)
            {
                int t = xa; xa = xb; xb = t;
            }
            drawHLine(xa, y, xb - xa + 1, c);
        }
    }

    void Canvas::drawRoundRect(const Rect &r, int radius, color_t c)
    {
        int rad = clampRadius(radius, r);
        if (rad <= 0)
        {
            drawRect(r, c);
            return;
        }

        // straight edges between the corners
        drawHLine(r.x + rad, r.y, r.w - 2 * rad, c);
        drawHLine(r.x + rad, (int)r.bottom() - 1, r.w - 2 * rad, c);
        drawVLine(r.x, r.y + rad, r.h - 2 * rad, c);
        drawVLine((int)r.right() - 1, r.y + rad, r.h - 2 * rad, c);

        // corner ring: inside the outer circle but not the one a pixel smaller
        for (int py = 0; py < rad; py++)
        {
            for (int px = 0; px < rad; px++)
            {
                if (cornerInside(px, py, rad, rad) && !cornerInside(px, py, rad, rad - 1))
                {
                    drawPixel(r.x + px, r.y + py, c);
                    drawPixel((int)r.right() - 1 - px, r.y + py, c);
                    drawPixel(r.x + px, (int)r.bottom() - 1 - py, c);
                    drawPixel((int)r.right() - 1 - px, (int)r.bottom() - 1 - py, c);
                }
            }
        }
    }

    void Canvas::fillCircle(int cx, int cy, int radius, color_t c)
    {
        if (radius < 0)
            return;
        int r2 = radius * radius + radius; // + r rounds the edge so small circles look round
        int x = radius;
        for (int dy = 0; dy <= radius; dy++)
        {
            while (x > 0 && x * x + dy * dy > r2)
                x--;
            drawHLine(cx - x, cy + dy, 2 * x + 1, c);
            if (dy)
                drawHLine(cx - x, cy - dy, 2 * x + 1, c);
        }
    }

    void Canvas::drawCircle(int cx, int cy, int radius, color_t c)
    {
        if (radius < 0)
            return;
        int x = radius, y = 0, err = 1 - radius;
        while (x >= y)
        {
            drawPixel(cx + x, cy + y, c);
            drawPixel(cx - x, cy + y, c);
            drawPixel(cx + x, cy - y, c);
            drawPixel(cx - x, cy - y, c);
            drawPixel(cx + y, cy + x, c);
            drawPixel(cx - y, cy + x, c);
            drawPixel(cx + y, cy - x, c);
            drawPixel(cx - y, cy - x, c);
            y++;
            if (err < 0)
            {
                err += 2 * y + 1;
            }
            else
            {
                x--;
                err += 2 * (y - x) + 1;
            }
        }
    }

    // ------------------------------------------------------------------
    // Blits
    // ------------------------------------------------------------------
    void Canvas::blitRaw(const color_t *src, int32_t srcStride, int16_t srcW, int16_t srcH,
                         const Rect &srcRect, Point dst, bool keyed, color_t key)
    {
        if (!_px || !src)
            return;

        Rect s = srcRect.intersect(Rect{0, 0, srcW, srcH});
        if (s.empty())
            return;

        // Where the (clipped) source lands, then trim to our clip and move the
        // source origin by the same amount.
        Rect d = Rect{(int16_t)(dst.x + (s.x - srcRect.x)), (int16_t)(dst.y + (s.y - srcRect.y)), s.w, s.h};
        Rect v = d.intersect(_clip);
        if (v.empty())
            return;

        int32_t sx = s.x + (v.x - d.x);
        int32_t sy = s.y + (v.y - d.y);
        for (int row = 0; row < v.h; row++)
        {
            const color_t *sp = src + (sy + row) * srcStride + sx;
            color_t *dp = _px + (int32_t)(v.y + row) * _stride + v.x;
            if (!keyed)
            {
                memmove(dp, sp, (size_t)v.w * sizeof(color_t));
            }
            else
            {
                for (int i = 0; i < v.w; i++)
                {
                    if (sp[i] != key)
                        dp[i] = sp[i];
                }
            }
        }
    }

    void Canvas::blit(const Canvas &src, Point dst)
    {
        blitRaw(src._px, src._stride, src._w, src._h, src.bounds(), dst, false, 0);
    }

    void Canvas::blit(const Canvas &src, const Rect &srcRect, Point dst)
    {
        blitRaw(src._px, src._stride, src._w, src._h, srcRect, dst, false, 0);
    }

    void Canvas::blitKeyed(const Canvas &src, const Rect &srcRect, Point dst, color_t key)
    {
        blitRaw(src._px, src._stride, src._w, src._h, srcRect, dst, true, key);
    }

    void Canvas::blitPixels(const color_t *src, int16_t srcW, int16_t srcH, Point dst, int32_t srcStride)
    {
        blitRaw(src, srcStride ? srcStride : srcW, srcW, srcH, Rect{0, 0, srcW, srcH}, dst, false, 0);
    }

    // ------------------------------------------------------------------
    // Text
    // ------------------------------------------------------------------
    void Canvas::drawGlyph(int x, int y, char ch, color_t fg, color_t bg, bool opaque, int scale)
    {
        if (opaque)
            fillRect(Rect{(int16_t)x, (int16_t)y, (int16_t)(FONT_CELL_W * scale), (int16_t)(FONT_CELL_H * scale)}, bg);

        const uint8_t *g = font5x7Glyph(ch);
        for (int col = 0; col < FONT_COLS; col++)
        {
            // merge vertical runs of lit pixels into one rectangle
            int row = 0;
            while (row < FONT_ROWS)
            {
                if (!(g[col] & (1 << row)))
                {
                    row++;
                    continue;
                }
                int run = 1;
                while (row + run < FONT_ROWS && (g[col] & (1 << (row + run))))
                    run++;
                fillRect(Rect{(int16_t)(x + col * scale), (int16_t)(y + row * scale), (int16_t)scale, (int16_t)(run * scale)}, fg);
                row += run;
            }
        }
    }

    int Canvas::drawTextImpl(int x, int y, const char *s, color_t fg, color_t bg, bool opaque, int scale)
    {
        if (!s || scale < 1)
            return x;
        int cx = x;
        for (; *s; s++)
        {
            if (*s == '\n')
            {
                cx = x;
                y += FONT_CELL_H * scale;
                continue;
            }
            drawGlyph(cx, y, *s, fg, bg, opaque, scale);
            cx += FONT_CELL_W * scale;
        }
        return cx;
    }

    int Canvas::drawText(int x, int y, const char *s, color_t fg, int scale)
    {
        return drawTextImpl(x, y, s, fg, 0, false, scale);
    }

    int Canvas::drawTextOpaque(int x, int y, const char *s, color_t fg, color_t bg, int scale)
    {
        return drawTextImpl(x, y, s, fg, bg, true, scale);
    }

    int Canvas::textWidth(const char *s, int scale)
    {
        if (!s || scale < 1)
            return 0;
        int best = 0, n = 0;
        for (;; s++)
        {
            if (*s == '\n' || *s == '\0')
            {
                best = (int)imax(best, n ? n * FONT_CELL_W * scale - scale : 0); // no gap after the last glyph
                n = 0;
                if (*s == '\0')
                    break;
            }
            else
            {
                n++;
            }
        }
        return best;
    }

    int Canvas::textHeight(const char *s, int scale)
    {
        if (!s || !*s || scale < 1)
            return 0;
        int lines = 1;
        for (; *s; s++)
        {
            if (*s == '\n')
                lines++;
        }
        return lines * FONT_CELL_H * scale - scale;
    }
}
