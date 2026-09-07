#pragma once
#include "CardLayout.h"

namespace tc {
// M5GFX 0.2.27's PaperMono conversion scales the portrait sensor into the
// native landscape panel, then rotates it. Undo that mapping, as in the
// original project's AppController::normalizedTouch. Display dimensions
// make the same correction work in portrait and landscape.
inline bool normalizePaperTouch(int &x, int &y, int width, int height) {
  if (width < 2 || height < 2 || x < 0 || y < 0 || x >= width || y >= height)
    return false;
  const int maxX = width - 1, maxY = height - 1;
  const int visibleX = (y * maxX + maxY / 2) / maxY;
  const int visibleY = ((maxX - x) * maxY + maxX / 2) / maxX;
  x = visibleX;
  y = visibleY;
  return true;
}
constexpr CardRect bookBackRect(int width, bool paper) {
  return {width - (paper ? 130 : 88), paper ? 38 : 34,
          paper ? 110 : 72, paper ? 44 : 32};
}
constexpr CardRect menuHomeRect(int width, int height) {
  return {0, height - 44, width, 44};
}
inline int menuRowAt(int x, int y, int width, int top, int height, int count) {
  if (height <= 0 || x < 10 || x >= width - 10 || y < top ||
      y >= top + height * count)
    return -1;
  return (y - top) / height;
}
} // namespace tc
