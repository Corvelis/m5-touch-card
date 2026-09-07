#pragma once

namespace tc {
// Pixel-centre circle mask; shared by every card icon, without an image buffer.
inline int circleInset(int diameter, int row) {
  if (diameter <= 0 || row < 0 || row >= diameter)
    return 0;
  const int dy = 2 * row + 1 - diameter;
  int inset = 0;
  while (inset < diameter / 2) {
    const int dx = 2 * inset + 1 - diameter;
    if (dx * dx + dy * dy <= diameter * diameter)
      break;
    ++inset;
  }
  return inset;
}
} // namespace tc
