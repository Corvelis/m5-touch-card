#include "CircleCrop.h"
#include <cassert>
#include <initializer_list>

int main() {
  for (int size : {1, 2, 3, 28, 56, 64, 128, 256}) {
    for (int y = 0; y < size; ++y) {
      const int inset = tc::circleInset(size, y);
      assert(inset == tc::circleInset(size, size - 1 - y));
      for (int x = 0; x < size; ++x) {
        const int dx = 2 * x + 1 - size, dy = 2 * y + 1 - size;
        assert((x >= inset && x < size - inset) ==
               (dx * dx + dy * dy <= size * size));
      }
    }
  }
}
