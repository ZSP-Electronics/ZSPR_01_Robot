#pragma once

// Animated_Face includes TFT_eSPI for its legacy sprite path; the host test only uses the
// zGFX path, so the sprite just has to compile.
#include <stdint.h>

struct TFT_eSprite
{
    int width() { return 0; }
    int height() { return 0; }
    void fillSprite(uint16_t) {}
    void fillRect(int, int, int, int, uint16_t) {}
    void drawLine(int, int, int, int, uint16_t) {}
    void fillTriangle(int, int, int, int, int, int, uint16_t) {}
    void fillCircleHelper(int, int, int, int, int, uint16_t) {}
};
