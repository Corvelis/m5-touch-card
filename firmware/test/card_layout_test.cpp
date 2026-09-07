#include "CardLayout.h"
#include <cassert>
#include <initializer_list>

namespace {
bool overlaps(tc::CardRect a, tc::CardRect b) {
  return a.x < b.x + b.w && b.x < a.x + a.w &&
         a.y < b.y + b.h && b.y < a.y + a.h;
}
void checkBounds(tc::CardRect box) {
  assert(box.w > 0 && box.h > 0);
  assert(box.x >= 16 && box.x + box.w <= 784);
  assert(box.y >= 44 && box.y + box.h <= 436);
}
}
int main() {
  for (bool horizontal : {false, true}) for (int design = 0; design < 3; ++design) {
    const auto l = tc::paperCardLayout(design, horizontal);
    const tc::CardRect boxes[] = {l.name, l.avatar, l.account, l.email, l.comment, l.qr};
    for (auto b : boxes) {
      assert(b.x >= 16 && b.y >= 30 && b.w > 0 && b.h > 0);
      assert(b.x + b.w <= (horizontal ? 784 : 464));
      assert(b.y + b.h <= (horizontal ? 436 : 756));
    }
    for (int i = 0; i < 6; ++i) for (int j = i + 1; j < 6; ++j)
      assert(!overlaps(boxes[i], boxes[j]));
    assert(l.namePx >= 52);
    assert(l.avatar.w >= 188 && l.avatar.h == l.avatar.w);
    if (design == 1) {
      assert(l.band.contains(l.name.x, l.name.y));
      assert(l.band.contains(l.avatar.x + l.avatar.w - 1, l.avatar.y + l.avatar.h - 1));
    }
  }
  using S = tc::UiScreen;
  // All six design/orientation combinations use the same rotation policy;
  // neither photo-home mode nor StackChan is ever affected.
  for (int design = 0; design < 3; ++design) {
    const auto layout = tc::landscapeLayout(design);
    const tc::CardRect elements[] = {
      layout.name, layout.avatar, layout.account, layout.email,
      layout.comment, layout.qr
    };
    for (const auto &box : elements)
      checkBounds(box);
    for (int a = 0; a < 6; ++a)
      for (int b = a + 1; b < 6; ++b)
        assert(!overlaps(elements[a], elements[b]));
    assert(layout.avatar.w == layout.avatar.h);
    assert(layout.qr.w == 168 && layout.qr.h == 168);
    const auto &q = layout.qr;
    assert(q.contains(q.x, q.y));
    assert(q.contains(q.x + q.w - 1, q.y + q.h - 1));
    assert(!q.contains(q.x - 1, q.y));
    assert(!q.contains(q.x + q.w, q.y));
    assert(!q.contains(q.x, q.y + q.h));
    assert(!q.contains(40, 455)); // Footer is not a QR hit.
    if (design == 1) {
      assert(layout.band.contains(layout.name.x, layout.name.y));
      assert(layout.band.contains(layout.name.x + layout.name.w - 1,
                                  layout.name.y + layout.name.h - 1));
      assert(layout.band.contains(layout.avatar.x + layout.avatar.w - 1,
                                  layout.avatar.y + layout.avatar.h - 1));
    }
    for (bool orientation : {false, true}) {
      for (int screen = int(S::Home); screen <= int(S::Format); ++screen) {
        auto s = static_cast<S>(screen);
        for (int home = 0; home < 3; ++home) {
          bool expected = orientation &&
            ((s == S::Home && home == 0) || s == S::Card ||
             s == S::Preview || s == S::Qr);
          assert(tc::landscapeCard(true, orientation, s, home) == expected);
          assert(!tc::landscapeCard(false, orientation, s, home));
        }
      }
    }
  }
  assert((!tc::CardRect{0, 0, 0, 0}.contains(0, 0)));
}
