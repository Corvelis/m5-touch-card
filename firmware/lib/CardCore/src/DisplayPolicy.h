#pragma once
#include "CardLayout.h"

namespace tc {
enum class PaperRefresh { Clean, Gray, Differential };
constexpr bool hasGrayContent(UiScreen screen) {
  return screen == UiScreen::Home || screen == UiScreen::Card ||
         screen == UiScreen::Preview || screen == UiScreen::Book;
}
constexpr PaperRefresh paperRefresh(bool initialized, UiScreen screen) {
  return !initialized ? PaperRefresh::Clean
                      : hasGrayContent(screen) ? PaperRefresh::Gray
                                               : PaperRefresh::Differential;
}
constexpr int paperMenuFontSize = 32;
constexpr bool hidePaperFooter(bool isPaper, bool lightOff, UiScreen screen, uint8_t home) {
  // Full-screen photographs have no controls: never erase their bottom edge.
  return isPaper && lightOff && !(screen == UiScreen::Home && home == 2);
}
constexpr CardRect paperFooterRect(int width, int height) {
  return {0, height - 44, width, 44};
}
constexpr bool minuteStatusDue(uint32_t now, uint32_t lastUpdate,
                               int minute, int lastMinute, bool busy) {
  return !busy && (minute != lastMinute || uint32_t(now - lastUpdate) >= 60000);
}
constexpr int paperMenuTop = 110;
constexpr int paperMenuRowHeight = 94;
} // namespace tc
