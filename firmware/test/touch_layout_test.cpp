#include "TouchLayout.h"
#include "DisplayPolicy.h"
#include <cassert>
#include <cmath>
#include <initializer_list>

// Model the pinned driver: portrait raw sensor -> native 800x480 affine
// scaling -> panel rotation (logical rotation + offset_rotation=3).
void converted(int rawX, int rawY, int rotation, int &x, int &y) {
  int tx = int(rawX * 799.0 / 479.0);
  int ty = int(rawY * 479.0 / 799.0);
  switch (rotation) {
  case 0: x = 479 - ty; y = tx; break;
  case 1: x = tx; y = ty; break;
  case 2: x = ty; y = 799 - tx; break;
  default: x = 799 - tx; y = 479 - ty; break;
  }
}
int main() {
  for (int r = 0; r < 4; ++r) {
    const int w = r % 2 ? 800 : 480, h = r % 2 ? 480 : 800;
    for (int rawX = 0; rawX < 480; rawX += 13)
      for (int rawY = 0; rawY < 800; rawY += 17) {
        int x, y;
        converted(rawX, rawY, r, x, y);
        assert(tc::normalizePaperTouch(x, y, w, h));
        const int expectedX = r == 0 ? rawX : r == 1 ? rawY :
                              r == 2 ? 479 - rawX : 799 - rawY;
        const int expectedY = r == 0 ? rawY : r == 1 ? 479 - rawX :
                              r == 2 ? 799 - rawY : rawX;
        assert(std::abs(x - expectedX) <= 2);
        assert(std::abs(y - expectedY) <= 2);
      }
  }
  for (int row = 0; row < 5; ++row) {
    const int center = tc::paperMenuTop + row * tc::paperMenuRowHeight +
                       tc::paperMenuRowHeight / 2;
    int x, y;
    converted(48, center, 0, x, y);
    assert(tc::normalizePaperTouch(x, y, 480, 800));
    assert(tc::menuRowAt(x, y, 480, tc::paperMenuTop,
                         tc::paperMenuRowHeight, 5) == row);
  }
  for (int rawX : {0, 479})
    for (int rawY : {0, 799}) {
      int x, y;
      converted(rawX, rawY, 0, x, y);
      assert(tc::normalizePaperTouch(x, y, 480, 800));
      assert(x == rawX && y == rawY);
    }
  assert(tc::menuRowAt(0, 200, 480, 110, 94, 5) == -1);
  assert(tc::menuRowAt(40, 109, 480, 110, 94, 5) == -1);
  assert(tc::menuRowAt(40, 580, 480, 110, 94, 5) == -1);
  int x = -1, y = 80;
  assert(!tc::normalizePaperTouch(x, y, 480, 800));
  for (bool paper : {true, false}) {
    const int width = paper ? 480 : 320, height = paper ? 800 : 240;
    const auto home = tc::menuHomeRect(width, height);
    assert(home.contains(20, height - 22));
    assert(home.contains(width - 1, height - 1));
    assert(!home.contains(20, height - 45));
    assert(!home.contains(width, height - 22));
    assert(!home.contains(20, height));
    const int top = paper ? tc::paperMenuTop : 64;
    const int rowHeight = paper ? tc::paperMenuRowHeight : 33;
    assert(top + 4 * rowHeight <= home.y);
    if (paper) {
      int tx, ty;
      converted(80, height - 22, 0, tx, ty);
      assert(tc::normalizePaperTouch(tx, ty, width, height));
      assert(home.contains(tx, ty));
    }
    const auto back = tc::bookBackRect(paper ? 480 : 320, paper);
    assert(back.y + back.h < (paper ? 100 : 72));
    assert(back.contains(back.x + back.w / 2, back.y + back.h / 2));
  }
}
