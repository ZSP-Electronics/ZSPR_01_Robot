#pragma once

// Just enough Arduino for Animated_Face on a PC. Time only moves when the test sets g_millis.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern unsigned long g_millis;
inline unsigned long millis() { return g_millis; }
inline long random(long max) { return max > 0 ? rand() % max : 0; }
inline long random(long mn, long mx) { return mx > mn ? mn + rand() % (mx - mn) : mn; }

struct SerialStub
{
    template <class... A>
    void printf(const char *f, A... a) { ::printf(f, a...); }
    template <class T>
    void print(const T &) {}
    template <class T>
    void println(const T &) {}
};
[[maybe_unused]] static SerialStub Serial;
