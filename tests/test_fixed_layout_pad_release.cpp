#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
struct circlePosition { int dx, dy; };
static circlePosition pad = {0, 0}, stick = {0, 0};
void hidCircleRead(circlePosition *p) { *p = pad; }
void hidCstickRead(circlePosition *p) { *p = stick; }
struct App {
  bool touch = false;
  int orientation = 0;
  bool IsPdfTouchDragActive() const { return touch; }
  bool IsNew3dsDevice() const { return true; }
};
struct Book { bool interaction = false; bool can_move = true; };
struct Text {};
static int draws = 0, smooth_draws = 0, delays = 0;
namespace orientation_utils { bool IsTurnedRight(int n) { return n != 0; } }
namespace book_renderer {
bool TranslateFixedLayoutViewport(Book *b, float, float) { return b->can_move; }
void SetFixedLayoutViewportInteraction(Book *b, bool active) { b->interaction = active; }
}
namespace book_nav {
void DrawPage(Book *b, Text *) { ++draws; if (!b->interaction) ++smooth_draws; }
}
#include "pad_helpers.inc"
static bool Frame(App &app, Book *book, Text *ts) {
  bool status_dirty = false;
  const auto delay_deferred = [&]() { ++delays; };
  // The preceding touch-release path clears this flag on non-touch frames.
  if (!app.touch) book->interaction = false;
#include "pad_block.inc"
  return status_dirty;
}
int main() {
  App app; Book book; Text text;
  assert(!Frame(app, &book, &text));
  pad.dx = 60;
  assert(Frame(app, &book, &text));
  assert(Frame(app, &book, &text));
  assert(draws == 2 && smooth_draws == 0);
  pad.dx = 0;
  assert(Frame(app, &book, &text) && "pad release must redraw with smoothing");
  assert(smooth_draws == 1 && delays == 3);
  assert(!Frame(app, &book, &text) && "idle must not redraw repeatedly");
  stick.dy = 30;
  assert(Frame(app, &book, &text));
  book.can_move = false; // At the page edge, no movement but still held.
  assert(!Frame(app, &book, &text));
  stick.dy = 0;
  assert(Frame(app, &book, &text));
  assert(smooth_draws == 2);
  pad.dx = 10; // Inside the dead zone.
  assert(!Frame(app, &book, &text));
  book.can_move = true; pad.dx = 60;
  assert(Frame(app, &book, &text));
  app.touch = true;
  assert(!Frame(app, &book, &text)); // Stylus owns the viewport.
  app.touch = false; pad.dx = 0;
  assert(!Frame(app, &book, &text)); // No stale pad-release redraw after touch.
  puts("PASS: pad/C-stick release redraws smoothing once, including at edges");
}
