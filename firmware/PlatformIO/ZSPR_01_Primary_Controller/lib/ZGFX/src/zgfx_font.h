#pragma once

#include <stdint.h>

namespace zgfx
{
    constexpr int FONT_COLS = 5;      // glyph pixels wide
    constexpr int FONT_ROWS = 7;      // glyph pixels tall
    constexpr int FONT_CELL_W = 6;    // advance: glyph + 1px gap
    constexpr int FONT_CELL_H = 8;    // line height: glyph + 1px gap

    // 5 column bytes for `ch` (bit 0 = top row). Anything outside ASCII 32..126 draws as '?'.
    const uint8_t *font5x7Glyph(char ch);
}
