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
// One cleaning request per observed local :00 minute. Retain a pending request
// while NFC/saving/drawing is busy; coalesce missed hours, never queue flashes.
struct PaperHourlyRefresh {
  bool pending = false, scheduled = false;
  int64_t scheduledHour = 0;
  void observe(bool enabled, int year, int dayOfYear, int hour, int minute) {
    if (!enabled) { pending = false; return; }
    if (minute != 0) return;
    // An identity, not an elapsed-time calculation. Include the year so the
    // same hour on New Year's Day cannot collide with the preceding year.
    const int64_t key = (int64_t(year) * 366 + dayOfYear) * 24 + hour;
    if (!scheduled || key != scheduledHour) {
      scheduled = true;
      scheduledHour = key;
      pending = true;
    }
  }
  bool due(bool busy) const { return pending && !busy; }
  void completed() { pending = false; }
};

// A clean waveform covers the retained frame, so batch clock/date/battery into
// one transaction. Differential updates keep the battery in its own rectangle
// to avoid including the photograph in a combined bounding box.
template <typename Display, typename DrawStatus, typename DrawBattery>
void paintPaperMinute(Display &display, bool clean, bool batteryChanged,
                      DrawStatus drawStatus, DrawBattery drawBattery) {
  display.startWrite();
  drawStatus();
  if (clean && batteryChanged) drawBattery();
  display.endWrite();
  if (!clean && batteryChanged) {
    display.startWrite();
    drawBattery();
    display.endWrite();
  }
}
constexpr int paperMenuTop = 110;
constexpr int paperMenuRowHeight = 94;
} // namespace tc
