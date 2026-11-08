// Host-side test for zgfx. No framework, no firmware:
//   g++ -std=c++11 -Wall -Wextra -I src src/*.cpp test/host_test.cpp -o zgfx_test && ./zgfx_test
//
// Not part of the PlatformIO build (the library's srcDir is src/).

#include "zGFX.h"
#include <stdio.h>
#include <string.h>
#include <vector>

using namespace zgfx;

static int g_checks = 0;
static int g_failed = 0;

// variadic so braces with commas (Rect{...}) can appear in the condition
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

// A canvas surrounded by a canary border: any write outside the visible area is caught.
struct Guarded
{
    enum : int { PAD = 4, CANARY = 0xDEAD };
    int w, h;
    std::vector<color_t> mem;
    Guarded(int w_, int h_) : w(w_), h(h_), mem((size_t)(w_ + 2 * PAD) * (h_ + 2 * PAD), (color_t)CANARY)
    {
        Canvas c = canvas();
        c.clear(0);
    }
    Canvas canvas()
    {
        int stride = w + 2 * PAD;
        return Canvas(mem.data() + PAD * stride + PAD, (int16_t)w, (int16_t)h, stride);
    }
    bool borderIntact() const
    {
        int stride = w + 2 * PAD;
        for (int y = 0; y < h + 2 * PAD; y++)
        {
            for (int x = 0; x < stride; x++)
            {
                bool inside = x >= PAD && x < PAD + w && y >= PAD && y < PAD + h;
                if (!inside && mem[(size_t)y * stride + x] != CANARY)
                    return false;
            }
        }
        return true;
    }
};

static int countPixels(const Canvas &c, color_t v)
{
    int n = 0;
    for (int y = 0; y < c.height(); y++)
        for (int x = 0; x < c.width(); x++)
            n += c.getPixel(x, y) == v;
    return n;
}

static void testTypes()
{
    CHECK(rgb(255, 255, 255) == 0xFFFF);
    CHECK(rgb(255, 0, 0) == colors::Red);
    CHECK(rgb(0, 255, 0) == colors::Green);
    CHECK(rgb(0, 0, 255) == colors::Blue);
    CHECK(rgb(64, 64, 64) == colors::DarkGray);
    CHECK(rgb(128, 128, 128) == colors::Gray);
    CHECK(rgb(192, 192, 192) == colors::LightGray);

    Rect a{0, 0, 10, 10}, b{5, 5, 10, 10}, c{20, 20, 5, 5};
    CHECK(a.intersects(b) && !a.intersects(c));
    CHECK((a.intersect(b) == Rect{5, 5, 5, 5}));
    CHECK(a.intersect(c).empty());
    CHECK((a.unite(c) == Rect{0, 0, 25, 25}));
    CHECK((Rect{0, 0, 0, 0}.unite(c) == c));
    CHECK(!(Rect{0, 0, 10, 10}.contains(10, 5)) && Rect{0, 0, 10, 10}.contains(9, 9));
}

static void testPrimitives()
{
    Guarded g(32, 24);
    Canvas c = g.canvas();

    // everything below deliberately pokes outside the canvas
    c.fillRect(Rect{-10, -10, 20, 20}, 1);
    CHECK(c.getPixel(0, 0) == 1 && c.getPixel(9, 9) == 1 && c.getPixel(10, 10) == 0);
    c.drawLine(-20, -20, 60, 50, 2);
    c.drawLine(31, 0, 0, 23, 3);
    c.drawCircle(0, 0, 15, 4);
    c.fillCircle(31, 23, 12, 5);
    c.drawRoundRect(Rect{-5, 3, 50, 40}, 8, 6);
    c.fillRoundRect(Rect{-5, 3, 50, 40}, 8, 7);
    c.drawText(-10, 20, "clipped text", 8, 2);
    c.drawHLine(-5, 5, 100, 9);
    c.drawVLine(5, -5, 100, 9);
    CHECK(g.borderIntact());

    // lines
    Guarded l(16, 16);
    Canvas lc = l.canvas();
    lc.drawLine(0, 0, 10, 10, 1);
    CHECK(countPixels(lc, 1) == 11 && lc.getPixel(0, 0) == 1 && lc.getPixel(10, 10) == 1 && lc.getPixel(5, 5) == 1);
    lc.clear(0);
    lc.drawLine(10, 3, 2, 3, 1); // reversed horizontal
    CHECK(countPixels(lc, 1) == 9);
    lc.clear(0);
    lc.drawLine(4, 12, 4, 2, 1); // reversed vertical
    CHECK(countPixels(lc, 1) == 11);

    // outline rect: 4 sides, interior empty, even for a 1-pixel-tall rect
    lc.clear(0);
    lc.drawRect(Rect{2, 2, 6, 5}, 1);
    CHECK(countPixels(lc, 1) == 2 * 6 + 2 * 3);
    CHECK(lc.getPixel(4, 4) == 0);
    lc.clear(0);
    lc.drawRect(Rect{2, 2, 6, 1}, 1);
    CHECK(countPixels(lc, 1) == 6);

    // circles: symmetric, centre filled, outline hollow
    Guarded k(40, 40);
    Canvas kc = k.canvas();
    kc.fillCircle(20, 20, 10, 1);
    CHECK(kc.getPixel(20, 20) == 1 && kc.getPixel(30, 20) == 1 && kc.getPixel(20, 10) == 1 && kc.getPixel(31, 20) == 0);
    bool sym = true; // mirror about the centre column / row
    for (int y = 0; y < 40; y++)
        for (int d = 0; d <= 20; d++)
            sym = sym && kc.getPixel(20 + d, y) == kc.getPixel(20 - d, y) && kc.getPixel(y, 20 + d) == kc.getPixel(y, 20 - d);
    CHECK(sym);
    kc.clear(0);
    kc.drawCircle(20, 20, 10, 1);
    CHECK(kc.getPixel(30, 20) == 1 && kc.getPixel(20, 20) == 0 && kc.getPixel(10, 20) == 1);

    // round rects: corner pixel cut off, edges and middle present
    Guarded r(30, 20);
    Canvas rc = r.canvas();
    rc.fillRoundRect(Rect{0, 0, 30, 20}, 6, 1);
    CHECK(rc.getPixel(0, 0) == 0 && rc.getPixel(29, 0) == 0 && rc.getPixel(0, 19) == 0 && rc.getPixel(29, 19) == 0);
    CHECK(rc.getPixel(15, 0) == 1 && rc.getPixel(0, 10) == 1 && rc.getPixel(15, 10) == 1);
    rc.clear(0);
    rc.drawRoundRect(Rect{0, 0, 30, 20}, 6, 1);
    CHECK(rc.getPixel(0, 0) == 0 && rc.getPixel(15, 0) == 1 && rc.getPixel(0, 10) == 1 && rc.getPixel(15, 10) == 0);
    // radius 0 and oversize radius degrade gracefully
    rc.clear(0);
    rc.fillRoundRect(Rect{0, 0, 30, 20}, 0, 1);
    CHECK(countPixels(rc, 1) == 600);
    rc.clear(0);
    rc.fillRoundRect(Rect{0, 0, 30, 20}, 999, 1);
    CHECK(rc.getPixel(15, 10) == 1);
    CHECK(r.borderIntact());
}

static void testClipAndSub()
{
    Guarded g(20, 20);
    Canvas c = g.canvas();
    c.setClip(Rect{5, 5, 10, 10});
    c.fillRect(Rect{0, 0, 20, 20}, 1);
    CHECK(countPixels(c, 1) == 100 && c.getPixel(5, 5) == 1 && c.getPixel(4, 5) == 0 && c.getPixel(15, 15) == 0);
    c.clear(2); // clear fills only the clip
    CHECK(countPixels(c, 2) == 100);
    c.resetClip();
    c.fillRect(Rect{0, 0, 20, 20}, 0);

    Canvas s = c.sub(Rect{10, 10, 20, 20}); // hangs off the edge -> trimmed to 10x10
    CHECK(s.width() == 10 && s.height() == 10);
    s.fillRect(Rect{0, 0, 3, 3}, 7);
    CHECK(c.getPixel(10, 10) == 7 && c.getPixel(12, 12) == 7 && c.getPixel(13, 13) == 0 && c.getPixel(9, 9) == 0);
    s.fillRect(Rect{-5, -5, 100, 100}, 8); // sub canvas clips to itself, not to its parent
    CHECK(c.getPixel(9, 9) == 0 && c.getPixel(19, 19) == 8);
    CHECK(!c.sub(Rect{50, 50, 5, 5}).valid());
    CHECK(g.borderIntact());
}

static void testBlit()
{
    Guarded sg(8, 8), dg(16, 16);
    Canvas src = sg.canvas(), dst = dg.canvas();
    src.clear(0x1111);
    src.fillRect(Rect{2, 2, 4, 4}, 0x2222);

    dst.blit(src, Point{4, 4});
    CHECK(dst.getPixel(4, 4) == 0x1111 && dst.getPixel(6, 6) == 0x2222 && dst.getPixel(11, 11) == 0x1111 && dst.getPixel(12, 12) == 0);

    dst.clear(0);
    dst.blitKeyed(src, Rect{0, 0, 8, 8}, Point{4, 4}, 0x1111); // key = the background colour
    CHECK(dst.getPixel(4, 4) == 0 && dst.getPixel(6, 6) == 0x2222 && countPixels(dst, 0x2222) == 16);

    dst.clear(0);
    dst.blit(src, Rect{2, 2, 4, 4}, Point{-1, -1}); // partly off the top-left
    CHECK(dst.getPixel(0, 0) == 0x2222 && dst.getPixel(2, 2) == 0x2222 && dst.getPixel(3, 3) == 0);
    dst.blit(src, Point{12, 12}); // partly off the bottom-right
    dst.blit(src, Point{100, 100});
    dst.blit(src, Point{-100, -100});

    color_t raw[6] = {1, 2, 3, 4, 5, 6};
    dst.clear(0);
    dst.blitPixels(raw, 3, 2, Point{1, 1});
    CHECK(dst.getPixel(1, 1) == 1 && dst.getPixel(3, 1) == 3 && dst.getPixel(1, 2) == 4 && dst.getPixel(3, 2) == 6);

    // overlapping blit inside one canvas
    dst.clear(0);
    dst.fillRect(Rect{0, 0, 4, 4}, 9);
    dst.blit(dst, Rect{0, 0, 4, 4}, Point{2, 2});
    CHECK(dst.getPixel(5, 5) == 9);
    CHECK(sg.borderIntact() && dg.borderIntact());
}

static void testText()
{
    CHECK(Canvas::textWidth("") == 0);
    CHECK(Canvas::textWidth("A") == 5);
    CHECK(Canvas::textWidth("AB") == 11);
    CHECK(Canvas::textWidth("AB", 2) == 22);
    CHECK(Canvas::textWidth("A\nABC") == 17); // widest line
    CHECK(Canvas::textHeight("A") == 7);
    CHECK(Canvas::textHeight("A\nB") == 15);
    CHECK(Canvas::textHeight("A", 2) == 14);

    Guarded g(40, 20);
    Canvas c = g.canvas();
    int end = c.drawText(1, 1, "I", 1);
    CHECK(end == 7);
    CHECK(countPixels(c, 1) > 0);
    // 'I' in this font: lit pixels stay inside its 5x7 box
    bool inside = true;
    for (int y = 0; y < 20; y++)
        for (int x = 0; x < 40; x++)
            if (c.getPixel(x, y) == 1 && (x < 1 || x > 5 || y < 1 || y > 7))
                inside = false;
    CHECK(inside);

    c.clear(0);
    c.drawText(0, 0, "I", 3, 1); // transparent leaves the cell alone
    CHECK(c.getPixel(5, 0) == 0);
    c.clear(0);
    c.drawTextOpaque(0, 0, "I", 3, 4, 1); // opaque fills the whole 6x8 cell with bg
    CHECK(c.getPixel(5, 7) == 4 && c.getPixel(6, 0) == 0 && countPixels(c, 4) + countPixels(c, 3) == 6 * 8);

    // scale 2 doubles the glyph
    Guarded g1(20, 20), g2(20, 20);
    Canvas a = g1.canvas(), b = g2.canvas();
    a.drawText(0, 0, "A", 1, 1);
    b.drawText(0, 0, "A", 1, 2);
    CHECK(countPixels(b, 1) == 4 * countPixels(a, 1));

    // newline moves down a line and back to x
    c.clear(0);
    c.drawText(2, 0, "I\nI", 1, 1);
    CHECK(c.getPixel(4, 1) == 1 && c.getPixel(4, 9) == 1);

    // unprintable characters draw as '?', not garbage / out of range
    c.clear(0);
    c.drawText(0, 0, "\x01\x7f\xff", 1, 1);
    CHECK(countPixels(c, 1) > 0 && g.borderIntact());
}

static void testDirty()
{
    DirtyTracker t(100, 100);
    Rect out[DirtyTracker::MAX_RECTS];
    CHECK(!t.pending() && t.redrawRects(out, 8) == 0);

    t.add(Rect{0, 0, 10, 10});
    t.add(Rect{5, 5, 10, 10}); // overlaps -> merged
    CHECK(t.redrawRects(out, 8) == 1 && (out[0] == Rect{0, 0, 15, 15}));

    t.add(Rect{50, 50, 5, 5}); // disjoint -> separate
    CHECK(t.redrawRects(out, 8) == 2);

    t.add(Rect{-20, -20, 500, 500}); // clipped to the bounds, swallows everything
    CHECK(t.redrawRects(out, 8) == 1 && (out[0] == Rect{0, 0, 100, 100}));

    // last frame's changes stay in the redraw set for exactly one more frame
    t.reset();
    t.add(Rect{10, 10, 5, 5});
    t.nextFrame();
    t.add(Rect{60, 60, 5, 5});
    CHECK(t.redrawRects(out, 8) == 2);
    t.nextFrame();
    CHECK(t.redrawRects(out, 8) == 1 && (out[0] == Rect{60, 60, 5, 5}));
    t.nextFrame();
    CHECK(!t.pending());

    // a full list folds into fewer rects instead of overflowing
    t.reset();
    for (int i = 0; i < 20; i++)
        t.add(Rect{(int16_t)(i * 5), (int16_t)(i * 5), 2, 2});
    uint8_t n = t.redrawRects(out, 8);
    CHECK(n >= 1 && n <= DirtyTracker::MAX_RECTS);
    for (int i = 0; i < 20; i++)
    {
        bool covered = false;
        for (uint8_t j = 0; j < n; j++)
            covered = covered || (out[j].contains(i * 5, i * 5) && out[j].contains(i * 5 + 1, i * 5 + 1));
        CHECK(covered);
    }
    // caller buffer smaller than the list: still covers everything
    uint8_t one = t.redrawRects(out, 1);
    CHECK(one == 1 && out[0].contains(95, 95) && out[0].contains(0, 0));
}

// ------------------------------------------------------------------ widgets
struct ClickCounter
{
    int clicks = 0;
    static void cb(void *u) { ++static_cast<ClickCounter *>(u)->clicks; }
};

static void testTouch()
{
    Screen s(100, 60);
    Button b1(Rect{0, 0, 40, 20}, "A"), b2(Rect{50, 0, 40, 20}, "B");
    ClickCounter c1, c2;
    b1.setOnClick(&ClickCounter::cb, &c1);
    b2.setOnClick(&ClickCounter::cb, &c2);
    CHECK(s.add(&b1) && s.add(&b2) && !s.add(&b1));

    // tap inside
    s.touch(TouchEvent{TouchEvent::Down, 10, 10, 0});
    CHECK(b1.pressed());
    s.touch(TouchEvent{TouchEvent::Up, 12, 11, 0});
    CHECK(!b1.pressed() && c1.clicks == 1);

    // press, drag off the button (captured: still the same widget), release outside -> no click
    s.touch(TouchEvent{TouchEvent::Down, 10, 10, 0});
    s.touch(TouchEvent{TouchEvent::Move, 80, 50, 0});
    CHECK(!b1.pressed() && !b2.pressed());
    s.touch(TouchEvent{TouchEvent::Up, 80, 50, 0});
    CHECK(c1.clicks == 1 && c2.clicks == 0);

    // drag off and back on -> still clicks
    s.touch(TouchEvent{TouchEvent::Down, 10, 10, 0});
    s.touch(TouchEvent{TouchEvent::Move, 80, 50, 0});
    s.touch(TouchEvent{TouchEvent::Move, 20, 10, 0});
    CHECK(b1.pressed());
    s.touch(TouchEvent{TouchEvent::Up, 20, 10, 0});
    CHECK(c1.clicks == 2);

    // two fingers, two buttons, independent
    s.touch(TouchEvent{TouchEvent::Down, 10, 10, 3});
    s.touch(TouchEvent{TouchEvent::Down, 60, 10, 4});
    CHECK(b1.pressed() && b2.pressed());
    s.touch(TouchEvent{TouchEvent::Up, 60, 10, 4});
    CHECK(b1.pressed() && !b2.pressed() && c2.clicks == 1);
    s.touch(TouchEvent{TouchEvent::Up, 10, 10, 3});
    CHECK(c1.clicks == 3);

    // touching empty space or hidden widgets does nothing; stray Move/Up are ignored
    s.touch(TouchEvent{TouchEvent::Down, 90, 50, 0});
    s.touch(TouchEvent{TouchEvent::Up, 90, 50, 0});
    b1.setVisible(false);
    s.touch(TouchEvent{TouchEvent::Down, 10, 10, 0});
    s.touch(TouchEvent{TouchEvent::Up, 10, 10, 0});
    s.touch(TouchEvent{TouchEvent::Move, 10, 10, 9});
    CHECK(c1.clicks == 3);

    // topmost wins when widgets overlap
    Screen s2(100, 60);
    Button under(Rect{0, 0, 50, 50}, "u"), over(Rect{10, 10, 20, 20}, "o");
    ClickCounter cu, co;
    under.setOnClick(&ClickCounter::cb, &cu);
    over.setOnClick(&ClickCounter::cb, &co);
    s2.add(&under);
    s2.add(&over);
    s2.touch(TouchEvent{TouchEvent::Down, 15, 15, 0});
    s2.touch(TouchEvent{TouchEvent::Up, 15, 15, 0});
    CHECK(co.clicks == 1 && cu.clicks == 0);

    // removing a widget mid-touch drops the capture instead of dangling
    s2.touch(TouchEvent{TouchEvent::Down, 15, 15, 0});
    CHECK(s2.remove(&over));
    s2.touch(TouchEvent{TouchEvent::Up, 15, 15, 0});
    CHECK(co.clicks == 1);
}

// Reference image: every visible widget drawn over a cleared canvas, no dirty logic.
static void renderFull(Canvas &c, color_t bg, const std::vector<Widget *> &ws)
{
    c.resetClip();
    c.clear(bg);
    for (Widget *w : ws)
        if (w->visible())
            w->draw(c);
}

static bool sameImage(const Canvas &a, const Canvas &b)
{
    for (int y = 0; y < a.height(); y++)
        for (int x = 0; x < a.width(); x++)
            if (a.getPixel(x, y) != b.getPixel(x, y))
                return false;
    return true;
}

// Two buffers swapped each frame, partial redraws driven by the tracker. After every
// presented frame the visible buffer must equal a from-scratch render.
// scratchPx > 0 draws through a strip buffer of that many pixels (narrower than some
// areas, so both the strip path and the direct fallback run).
static void testDoubleBuffering(size_t scratchPx)
{
    std::vector<color_t> scratch(scratchPx ? scratchPx : 1);
    color_t *sp = scratchPx ? scratch.data() : nullptr;
    const int W = 120, H = 80;
    Guarded bufA(W, H), bufB(W, H), ref(W, H);
    Canvas canv[2] = {bufA.canvas(), bufB.canvas()};
    Canvas refc = ref.canvas();

    const color_t BG = rgb(10, 10, 30);
    Screen screen(W, H, BG);
    Label label(Rect{4, 4, 60, 8}, "start", colors::White, rgb(0, 0, 60));
    ProgressBar bar(Rect{4, 20, 100, 10});
    Button btn(Rect{10, 40, 50, 24}, "Go");
    Label mover(Rect{80, 50, 30, 8}, "x", colors::Yellow, BG);
    std::vector<Widget *> all = {&label, &bar, &btn, &mover};
    for (Widget *w : all)
        screen.add(w);

    int front = 0; // canv[front] is what's "on screen"
    bool allMatch = true;
    for (int frame = 0; frame < 60; frame++)
    {
        // change a few things; some frames change nothing
        if (frame % 3 == 0)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "frame %d", frame);
            label.setText(buf);
        }
        if (frame % 2 == 0)
            bar.setValue((frame * 7) % 101);
        if (frame % 11 == 5)
            btn.setLabel(frame % 2 ? "Stop" : "Go");
        if (frame % 4 == 1)
            mover.setBounds(Rect{(int16_t)(10 + (frame * 5) % 80), (int16_t)(50 + (frame % 5) * 4), 30, 8});
        if (frame == 20)
            screen.touch(TouchEvent{TouchEvent::Down, 20, 50, 0});
        if (frame == 25)
            screen.touch(TouchEvent{TouchEvent::Up, 20, 50, 0});
        if (frame == 40)
            btn.setVisible(false);
        if (frame == 50)
            btn.setVisible(true);

        int back = 1 - front;
        if (screen.render(canv[back], sp, scratchPx))
            front = back; // present
        screen.endFrame();

        renderFull(refc, BG, all);
        if (!sameImage(canv[front], refc))
        {
            allMatch = false;
            printf("double-buffer mismatch at frame %d\n", frame);
            break;
        }
    }
    CHECK(allMatch);
    CHECK(bufA.borderIntact() && bufB.borderIntact());

    // once everything is quiet, nothing is redrawn (no needless presents)
    screen.render(canv[1 - front]);
    screen.endFrame();
    screen.render(canv[1 - front]);
    screen.endFrame();
    CHECK(!screen.render(canv[1 - front]));
}

static void testPartialRedraw()
{
    // A change must only touch pixels inside the changed widget's area.
    const int W = 60, H = 40;
    Guarded g(W, H);
    Canvas c = g.canvas();
    Screen screen(W, H, 0);
    Label a(Rect{0, 0, 20, 8}, "a", 1, 2);
    ProgressBar far(Rect{30, 20, 20, 8});
    screen.add(&a);
    screen.add(&far);
    screen.render(c); // first frame draws everything
    screen.endFrame();
    screen.render(c); // catches up the "previous frame" half
    screen.endFrame();

    c.clear(0xABCD); // sentinel everywhere
    a.setText("b");
    CHECK(screen.render(c));
    CHECK(c.getPixel(5, 3) != 0xABCD);        // inside the label: redrawn
    CHECK(c.getPixel(40, 24) == 0xABCD);      // the bar was not touched
    CHECK(c.getPixel(59, 39) == 0xABCD);
}

// A moving compass needle must repaint only around the needle, and the partial
// redraws into two alternating buffers must match a from-scratch render.
static void testCompassPartial()
{
    Guarded a(96, 96), b(96, 96), ref(96, 96);
    Canvas buf[2] = {a.canvas(), b.canvas()};
    Canvas rc = ref.canvas();

    Compass k(Rect{0, 0, 96, 96});
    Screen s(96, 96, colors::Black);
    s.add(&k);
    k.setValid(true);

    bool allMatch = true;
    int32_t worstArea = 0;
    for (int i = 0; i < 80; i++)
    {
        if (i >= 3)
            k.setHeading((float)(i * 7)); // 7 degrees a frame, wraps several times
        Canvas &back = buf[i & 1];
        Rect rects[DirtyTracker::MAX_RECTS];
        uint8_t n = s.dirty().redrawRects(rects, DirtyTracker::MAX_RECTS);
        int32_t area = 0;
        for (uint8_t r = 0; r < n; r++)
            area += rects[r].area();
        if (i >= 5 && area > worstArea)
            worstArea = area;
        s.render(back);
        s.endFrame();

        // From scratch: a fresh widget at the same heading.
        Compass k2(Rect{0, 0, 96, 96});
        Screen s2(96, 96, colors::Black);
        s2.add(&k2);
        k2.setValid(true);
        k2.setHeading(k.heading());
        s2.render(rc);
        for (int y = 0; y < 96 && allMatch; y++)
            for (int x = 0; x < 96; x++)
                if (back.getPixel(x, y) != rc.getPixel(x, y))
                {
                    allMatch = false;
                    break;
                }
    }
    CHECK(allMatch);
    CHECK(worstArea > 0 && worstArea < 96 * 96 / 2); // a needle sweep, not the whole dial, per frame
    CHECK(a.borderIntact() && b.borderIntact() && ref.borderIntact());
}

static void testDials()
{
    const color_t sky = rgb(40, 120, 200), ground = rgb(140, 90, 40);
    Guarded g(96, 96);
    Canvas c = g.canvas();
    Screen screen(96, 96, colors::Black);
    Horizon h(Rect{0, 0, 96, 96});
    Compass k(Rect{0, 0, 96, 96});
    screen.add(&h);

    // Level: sky on top, ground on the bottom, both present.
    h.setValid(true);
    h.setAttitude(0, 0);
    screen.render(c);
    CHECK(c.getPixel(48, 20) == sky && c.getPixel(48, 75) == ground);
    CHECK(c.getPixel(0, 0) == colors::Black); // corner is outside the dial

    // Nose up: horizon moves down, so more sky.
    int groundLevel = countPixels(c, ground);
    h.setAttitude(20, 0);
    screen.endFrame();
    screen.render(c);
    CHECK(countPixels(c, ground) < groundLevel);

    // Roll right: right side has more ground than the left, at the same height.
    h.setAttitude(0, 30);
    screen.endFrame();
    screen.render(c);
    CHECK(c.getPixel(80, 40) == ground && c.getPixel(16, 40) == sky);
    h.setAttitude(0, -30);
    screen.endFrame();
    screen.render(c);
    CHECK(c.getPixel(16, 40) == ground && c.getPixel(80, 40) == sky);

    // Extremes never write outside the canvas.
    h.setAttitude(90, 180);
    screen.endFrame();
    screen.render(c);
    h.setAttitude(-90, -180);
    screen.endFrame();
    screen.render(c);
    CHECK(g.borderIntact());

    // Jitter below the step doesn't dirty the widget; a real change does.
    screen.endFrame();
    screen.endFrame();
    screen.render(c);
    h.setAttitude(-90.5f, -179.5f);
    CHECK(!screen.render(c));
    h.setAttitude(-80, -179.5f);
    CHECK(screen.render(c));

    // Compass: needle tip colour appears toward the heading.
    Guarded cg(96, 96);
    Canvas cc = cg.canvas();
    Screen s2(96, 96, colors::Black);
    s2.add(&k);
    k.setValid(true);
    k.setHeading(90); // east: tip to the right of centre
    s2.render(cc);
    int right = 0, left = 0;
    for (int y = 0; y < 96; y++)
        for (int x = 0; x < 96; x++)
            if (cc.getPixel(x, y) == colors::Red && y > 30 && y < 66)
                (x > 48 ? right : left)++;
    CHECK(right > 10 && left == 0);
    k.setHeading(-270); // wraps to 90: no change
    s2.endFrame();
    s2.endFrame();
    s2.render(cc);
    k.setHeading(90.5f);
    CHECK(!s2.render(cc)); // half a degree is under the step
    k.setHeading(359);
    k.setHeading(1); // 2 degrees apart across the wrap
    CHECK(k.heading() == 1.0f);
    CHECK(cg.borderIntact());
}

int main()
{
    testTypes();
    testPrimitives();
    testClipAndSub();
    testBlit();
    testText();
    testDirty();
    testTouch();
    testDoubleBuffering(0);
    testDoubleBuffering(37);
    testDoubleBuffering(300);
    testPartialRedraw();
    testDials();
    testCompassPartial();

    printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
