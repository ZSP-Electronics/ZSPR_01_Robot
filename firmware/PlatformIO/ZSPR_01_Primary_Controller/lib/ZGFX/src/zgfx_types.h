#pragma once

// zgfx: small RGB565 drawing library. No dependencies beyond <stdint.h>/<stddef.h>:
// no Arduino, no RTOS, no heap. Everything lives in namespace zgfx.

#include <stdint.h>
#include <stddef.h>

namespace zgfx
{
    // RGB565 in native (little-endian) uint16_t order -- the layout the ESP32 RGB LCD
    // driver scans out, so a canvas can point straight at its frame buffer.
    using color_t = uint16_t;

    constexpr color_t rgb(uint8_t r, uint8_t g, uint8_t b)
    {
        return (color_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }

    namespace colors
    {
        constexpr color_t Black = 0x0000;
        constexpr color_t White = 0xFFFF;
        constexpr color_t Red = 0xF800;
        constexpr color_t Green = 0x07E0;
        constexpr color_t Blue = 0x001F;
        constexpr color_t Yellow = 0xFFE0;
        constexpr color_t Cyan = 0x07FF;
        constexpr color_t Magenta = 0xF81F;
        constexpr color_t DarkGray = 0x4208;  // rgb(64,64,64)
        constexpr color_t Gray = 0x8410;      // rgb(128,128,128)
        constexpr color_t LightGray = 0xC618; // rgb(192,192,192)
    }

    // Named helpers instead of min/max: Arduino defines those as macros.
    inline int32_t imin(int32_t a, int32_t b) { return a < b ? a : b; }
    inline int32_t imax(int32_t a, int32_t b) { return a > b ? a : b; }

    struct Point
    {
        int16_t x;
        int16_t y;
    };

    // x/y is the top-left pixel; right() and bottom() are one past the last pixel.
    struct Rect
    {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;

        bool empty() const { return w <= 0 || h <= 0; }
        int32_t right() const { return (int32_t)x + w; }
        int32_t bottom() const { return (int32_t)y + h; }
        int32_t area() const { return empty() ? 0 : (int32_t)w * h; }

        bool contains(int32_t px, int32_t py) const
        {
            return px >= x && py >= y && px < right() && py < bottom();
        }

        // o lies entirely inside this rect (an empty o is contained by anything).
        bool contains(const Rect &o) const
        {
            return o.empty() || (!empty() && o.x >= x && o.y >= y && o.right() <= right() && o.bottom() <= bottom());
        }

        bool intersects(const Rect &o) const
        {
            return !empty() && !o.empty() && x < o.right() && o.x < right() && y < o.bottom() && o.y < bottom();
        }

        Rect intersect(const Rect &o) const
        {
            int32_t x0 = imax(x, o.x), y0 = imax(y, o.y);
            int32_t x1 = imin(right(), o.right()), y1 = imin(bottom(), o.bottom());
            if (x1 <= x0 || y1 <= y0)
                return Rect{0, 0, 0, 0};
            return Rect{(int16_t)x0, (int16_t)y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0)};
        }

        // Bounding box of both; an empty rect contributes nothing.
        Rect unite(const Rect &o) const
        {
            if (empty())
                return o;
            if (o.empty())
                return *this;
            int32_t x0 = imin(x, o.x), y0 = imin(y, o.y);
            int32_t x1 = imax(right(), o.right()), y1 = imax(bottom(), o.bottom());
            return Rect{(int16_t)x0, (int16_t)y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0)};
        }

        bool operator==(const Rect &o) const { return x == o.x && y == o.y && w == o.w && h == o.h; }
        bool operator!=(const Rect &o) const { return !(*this == o); }
    };

    // Same shape as the firmware's Display_Module TouchEvent, but this library
    // doesn't depend on it: the caller copies the fields across.
    struct TouchEvent
    {
        enum Type : uint8_t
        {
            Down,
            Move,
            Up
        };
        Type type;
        int16_t x;
        int16_t y;
        uint8_t id; // which finger; constant from Down to Up
    };
}
