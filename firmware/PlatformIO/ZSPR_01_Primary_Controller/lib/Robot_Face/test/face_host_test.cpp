// Host-side test for the zGFX face (RoboFaceWidget). No framework, no firmware. From lib/Robot_Face:
//   g++ -std=c++17 -Wall -fsanitize=address,undefined -I test/stub -I . -I ../ZGFX/src test/face_host_test.cpp *.cpp ../ZGFX/src/*.cpp -o face_test && ./face_test
//
// Not part of the PlatformIO build (test/ is excluded from library sources).
//
// The face is built in memory pre-filled with junk, the way `new` hands it out on the
// device, so a member left uninitialised shows up here instead of as stretched or
// flickering eyes on the panel.

#include "RoboFaceWidget.h"
#include <new>
#include <vector>

unsigned long g_millis = 0;

using namespace zgfx;

static int g_checks = 0;
static int g_failed = 0;

#define CHECK(...)                                                       \
    do                                                                   \
    {                                                                    \
        g_checks++;                                                      \
        if (!(__VA_ARGS__))                                              \
        {                                                                \
            g_failed++;                                                  \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #__VA_ARGS__); \
        }                                                                \
    } while (0)

static const int SCR_W = 480, SCR_H = 480;
static const int EYE_W = 130, EYE_H = 160;
static const int FRAME_MS = 46; // a UI frame that presents: two vsyncs at 12 MHz

// A RoboFaceWidget constructed over junk bytes, configured like UI_Home.
struct JunkFace
{
    alignas(RoboFaceWidget) unsigned char mem[sizeof(RoboFaceWidget)];
    RoboFaceWidget *w;

    explicit JunkFace(bool animate)
    {
        memset(mem, 0xA5, sizeof(mem));
        w = new (mem) RoboFaceWidget(Rect{0, 0, SCR_W, SCR_H});
        RGBroboFace &f = w->face();
        f.setEyeWidth(EYE_W);
        f.setEyeHeight(EYE_H);
        f.setEyeBorderradius(40);
        f.setEyeSpacebetween(20);
        if (animate)
        {
            f.setAutoblinker(true, 2, 4);
            f.setCuriosity(true);
            f.setPositionDistance(15);
        }
    }
    ~JunkFace() { w->~RoboFaceWidget(); }
};

// Bounding box of the non-background pixels.
static Rect paintedBox(const std::vector<color_t> &px, color_t bg)
{
    int x0 = SCR_W, y0 = SCR_H, x1 = -1, y1 = -1;
    for (int i = 0; i < SCR_W * SCR_H; i++)
    {
        if (px[i] == bg)
            continue;
        int x = i % SCR_W, y = i / SCR_W;
        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
    }
    if (x1 < 0)
        return Rect{0, 0, 0, 0};
    return Rect{(int16_t)x0, (int16_t)y0, (int16_t)(x1 - x0 + 1), (int16_t)(y1 - y0 + 1)};
}

// A still face must settle to the configured size, whatever the memory held before.
static void testStillFaceSize()
{
    srand(1);
    g_millis = 0;
    JunkFace jf(false);
    RGBroboFace &f = jf.w->face();
    for (int i = 0; i < 100; i++)
    {
        g_millis += FRAME_MS;
        jf.w->tick();
    }

    std::vector<color_t> px(SCR_W * SCR_H);
    Canvas c(px.data(), SCR_W, SCR_H);
    jf.w->draw(c);
    Rect box = paintedBox(px, f.backgroundColor());
    CHECK(box.h >= EYE_H - 4 && box.h <= EYE_H + 4);
    CHECK(box.w >= 2 * EYE_W && box.w <= 2 * EYE_W + 60);
    // centred on the screen
    CHECK(box.y + box.h / 2 > SCR_H / 2 - 10 && box.y + box.h / 2 < SCR_H / 2 + 10);

    // Nothing moves, so nothing is reported dirty.
    Rect d[Animated_Face::MAX_DIRTY];
    g_millis += FRAME_MS;
    f.tick();
    f.takeDirty(d);
    g_millis += FRAME_MS;
    f.tick();
    CHECK(f.takeDirty(d) == 0);
}

// The firmware's loop: tick, partial redraw into the back buffer, swap. After every
// presented frame the visible buffer must equal a from-scratch render, and the eyes must
// stay within their configured size while they blink and wander.
static void testDoubleBufferedAnimation()
{
    srand(2);
    g_millis = 0;
    JunkFace jf(true);
    RGBroboFace &f = jf.w->face();
    Screen screen(SCR_W, SCR_H, colors::Black);
    screen.add(jf.w);

    std::vector<color_t> fb[2] = {std::vector<color_t>(SCR_W * SCR_H, 0), std::vector<color_t>(SCR_W * SCR_H, 0)};
    std::vector<color_t> ref(SCR_W * SCR_H);
    std::vector<color_t> scratch(480 * 16); // the firmware's UI_SCRATCH_PIXELS
    int front = 0, presented = 0, mismatched = 0, tooBig = 0, minH = SCR_H, maxH = 0;

    for (int frame = 0; frame < 20000; frame++) // ~15 minutes
    {
        g_millis += FRAME_MS;
        jf.w->tick();
        int back = 1 - front;
        Canvas c(fb[back].data(), SCR_W, SCR_H);
        if (screen.render(c, scratch.data(), scratch.size()))
        {
            presented++;
            front = back;

            Canvas rc(ref.data(), SCR_W, SCR_H);
            rc.clear(colors::Black);
            jf.w->draw(rc);
            if (fb[front] != ref && mismatched++ < 5)
                printf("  frame %d (t=%lu ms): visible buffer differs from a full render\n", frame, g_millis);

            Rect box = paintedBox(ref, f.backgroundColor());
            if (box.h > maxH) maxH = box.h;
            if (box.h < minH) minH = box.h;
            if (box.h > EYE_H + 20 && tooBig++ < 5)
                printf("  frame %d: eyes %d px tall\n", frame, box.h);
        }
        screen.endFrame();
    }

    CHECK(presented > 1000); // it really animated
    CHECK(mismatched == 0);
    CHECK(tooBig == 0);
    CHECK(minH < 20); // blinks close the eyes
    CHECK(maxH >= EYE_H - 4);
}

int main()
{
    testStillFaceSize();
    testDoubleBufferedAnimation();

    printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
