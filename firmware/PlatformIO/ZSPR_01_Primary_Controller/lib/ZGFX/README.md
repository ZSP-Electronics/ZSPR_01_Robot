# ZGFX

Small RGB565 drawing library for a framebuffer display. Pure C++11: only `<stdint.h>`,
`<stddef.h>`, `<string.h>` and `<math.h>`. No Arduino, no RTOS, no heap, no globals. All of it is in
`namespace zgfx`; include `zGFX.h`.

## Pieces

| File | What |
|---|---|
| `zgfx_types.h` | `color_t` (RGB565), `rgb()`, `Point`, `Rect`, `TouchEvent` |
| `zgfx_canvas.h/.cpp` | `Canvas`: a non-owning view of a pixel buffer and every primitive |
| `zgfx_font.*` | built-in 5x7 font (ASCII 32..126), integer scale, no anti-aliasing |
| `zgfx_dirty.*` | `DirtyTracker`: redraw set for a double-buffered display |
| `zgfx_widgets.*` | `Widget`, `Label`, `Button`, `ProgressBar`, `Compass`, `Horizon`, and `Screen` (draw order, touch routing, rendering) |

## Canvas

A `Canvas` is `(pixels, width, height, stride)`. It does not own the memory, so it can wrap
the display's back frame buffer, a widget's cached sprite, or a small tile in internal RAM:

```cpp
zgfx::Canvas fb(backBuffer, 480, 480);          // stride defaults to width
fb.clear(zgfx::colors::Black);
fb.fillRoundRect({20, 20, 200, 60}, 8, zgfx::rgb(40, 60, 110));
fb.drawText(30, 40, "Hello", zgfx::colors::White, 2);
zgfx::Canvas tile = fb.sub({0, 0, 64, 64});     // shares the memory, own origin
```

Everything is clipped to the canvas and to `setClip()`, so callers never bounds-check.
Writes are cheap, reads are not (the frame buffer is in PSRAM): only `getPixel()` and the
source side of a keyed blit read.

## Double-buffered redraw

The buffer you draw into is two frames old. A partial redraw is only correct if it covers
this frame's changes **and** last frame's. `Screen` does that for you:

```cpp
zgfx::Screen screen(480, 480);
screen.add(&label);                       // widgets you keep alive
...
screen.touch(ev);                         // from the touch queue
label.setText("changed");                 // marks its area dirty
zgfx::Canvas back(display.backBuffer(), 480, 480);
if (screen.render(back))                  // redraws only the dirty areas
    display.present();
screen.endFrame();                        // after presenting
```

## Widgets

`draw(Canvas&)` draws in screen coordinates and is called with the canvas clipped to the
area being redrawn, so a widget just draws itself fully. Anything that changes the result
calls `invalidate()`. Touch goes to the topmost widget under a finger on `Down`; `Move` and
`Up` keep going to that widget (per finger id, up to 5) even if the finger leaves it.

## Test

No firmware needed:

```
g++ -std=c++11 -Wall -Wextra -fsanitize=address,undefined -I src src/*.cpp test/host_test.cpp -o zgfx_test && ./zgfx_test
```

It draws into canary-guarded buffers (catches out-of-bounds writes), checks primitives and
text, routes touches, and runs a 60-frame double-buffer simulation that compares the visible
buffer against a from-scratch render after every frame.

## Credits

`src/zgfx_font.cpp` is the classic 5x7 table from Adafruit_GFX (BSD).
