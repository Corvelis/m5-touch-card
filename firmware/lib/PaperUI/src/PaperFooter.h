#pragma once
#include "DisplayPolicy.h"
#include <M5GFX.h>
#include <array>

namespace tc {
// Cache only the reserved controls, never a stale copy of the clock/card.
// Capture before clearing inside the same display transaction, including when
// NFC completion changes the screen while the light is off.
class PaperFooterCache {
  std::array<lgfx::grayscale_t, 800 * 44> pixels{};
  int width = 0, height = 0;
public:
  void capture(lgfx::LGFXBase &d, UiScreen screen, uint8_t home) {
    width = height = 0;
    if (!hidePaperFooter(true, true, screen, home) || d.width() > 800) return;
    const auto b = paperFooterRect(d.width(), d.height());
    d.readRect(b.x, b.y, b.w, b.h, pixels.data());
    width = d.width(); height = d.height();
  }
  bool restore(lgfx::LGFXBase &d) const {
    if (!width || width != d.width() || height != d.height()) return false;
    const auto b = paperFooterRect(width, height);
    d.pushImage(b.x, b.y, b.w, b.h, pixels.data());
    return true;
  }
};
inline void clearSleepingPaperFooter(lgfx::LGFXBase &d, bool lightOff,
                                     UiScreen screen, uint8_t home) {
  if (!hidePaperFooter(true, lightOff, screen, home)) return;
  const auto b = paperFooterRect(d.width(), d.height());
  d.fillRect(b.x, b.y, b.w, b.h, 0xFFFFFFu);
}
} // namespace tc
