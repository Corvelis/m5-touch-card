#include "DisplayPolicy.h"
#include <cassert>
#include <initializer_list>
#include <string>

struct FakeMinuteDisplay {
  int depth = 0, flushes = 0;
  std::string events;
  void startWrite() { ++depth; }
  void endWrite() { assert(depth > 0); if (--depth == 0) { ++flushes; events += '|'; } }
};

int main() {
  using namespace tc;
  assert(paperRefresh(false, UiScreen::Home) == PaperRefresh::Clean);
  assert(paperRefresh(false, UiScreen::Menu) == PaperRefresh::Clean);
  for (auto screen : {UiScreen::Home, UiScreen::Card, UiScreen::Preview,
                      UiScreen::Book, UiScreen::Month, UiScreen::Nfc}) {
    assert(paperRefresh(false, screen) == PaperRefresh::Clean);
    assert(paperRefresh(true, screen) != PaperRefresh::Clean);
  }
  assert(!minuteStatusDue(59999, 0, 10, 10, false));
  assert(minuteStatusDue(60000, 0, 10, 10, false));
  assert(minuteStatusDue(1000, 0, 11, 10, false));
  assert(!minuteStatusDue(60000, 0, 11, 10, true));
  assert(minuteStatusDue(61000, 0, 11, 10, false)); // Busy: retry, do not consume.
  assert(minuteStatusDue(30000, UINT32_MAX - 30000, 10, 10, false));
  PaperHourlyRefresh hourly;
  hourly.observe(true, 126, 250, 8, 59);
  assert(!hourly.due(false));
  for (int hour = 9; hour <= 10; ++hour) {
    hourly.observe(true, 126, 250, hour, 0);
    assert(hourly.due(false) && !hourly.due(true));
    hourly.completed();
    for (int minute = 0; minute < 60; ++minute) {
      hourly.observe(true, 126, 250, hour, minute);
      assert(!hourly.due(false));
    }
  }
  // NFC busy at :00: keep one request and execute it after completion, even
  // after the minute changes. Busy does not consume or duplicate maintenance.
  hourly.observe(true, 126, 250, 11, 0);
  for (int minute = 0; minute < 10; ++minute) {
    hourly.observe(true, 126, 250, 11, minute);
    assert(!hourly.due(true) && hourly.pending);
  }
  assert(hourly.due(false));
  hourly.completed();
  hourly.observe(true, 126, 250, 11, 0); // Correct clock within the same hour.
  assert(!hourly.due(false));
  hourly.observe(true, 126, 250, 13, 37); // No unscheduled mid-hour clean.
  assert(!hourly.due(false));
  // Several blocked hours coalesce into one later refresh, not a flash queue.
  for (int hour = 14; hour <= 18; ++hour) hourly.observe(true, 126, 250, hour, 0);
  assert(hourly.due(false));
  hourly.completed();
  assert(!hourly.due(false));
  // Date/year rollover and leap-day hour identities remain distinct.
  for (int day = 0; day < 366; ++day) {
    hourly.observe(true, 128, day, 0, 0);
    assert(hourly.due(false));
    hourly.completed();
  }
  hourly.observe(true, 129, 0, 0, 0);
  assert(hourly.due(false));
  hourly.completed();
  hourly.observe(true, 129, 0, 0, 0);
  assert(!hourly.due(false));
  hourly.observe(true, 129, 0, 1, 0);
  assert(hourly.pending);
  hourly.observe(false, 129, 0, 1, 0); // Unknown clock or full-screen photo.
  assert(!hourly.pending);
  hourly.observe(true, 129, 0, 1, 0);
  assert(!hourly.pending); // Do not duplicate the same observed hour.
  PaperHourlyRefresh boot;
  boot.observe(true, 126, 250, 9, 0);
  boot.completed(); // Initial clean render counts for :00 maintenance.
  boot.observe(true, 126, 250, 9, 0);
  assert(!boot.due(false));
  for (bool clean : {false, true}) for (bool batteryChanged : {false, true}) {
    FakeMinuteDisplay d;
    paintPaperMinute(d, clean, batteryChanged, [&] {
      assert(d.depth == 1);
      // Nested date-region transactions must not flush a second clean frame.
      d.startWrite(); d.events += 'D'; d.endWrite();
      d.events += 'T';
    }, [&] { assert(d.depth == 1); d.events += 'B'; });
    assert(d.depth == 0 && d.flushes == (!clean && batteryChanged ? 2 : 1));
    assert(d.events == (batteryChanged ? (clean ? "DTB|" : "DT|B|") : "DT|"));
  }
  for (auto screen : {UiScreen::Home, UiScreen::Card, UiScreen::Preview,
                      UiScreen::Book})
    assert(paperRefresh(true, screen) == PaperRefresh::Gray);
  for (auto screen : {UiScreen::Menu, UiScreen::Settings, UiScreen::Design,
                      UiScreen::Orientation, UiScreen::Storage, UiScreen::Qr})
    for (int update = 0; update < 100; ++update)
      assert(paperRefresh(true, screen) == PaperRefresh::Differential);
  assert(paperMenuFontSize >= 32);
  for (auto screen : {UiScreen::Home, UiScreen::Card, UiScreen::Preview,
                      UiScreen::Book, UiScreen::Month, UiScreen::Nfc, UiScreen::Menu}) {
    for (uint8_t home = 0; home < 3; ++home) {
      assert(!hidePaperFooter(false, true, screen, home));
      assert(!hidePaperFooter(true, false, screen, home));
      assert(hidePaperFooter(true, true, screen, home) ==
             !(screen == UiScreen::Home && home == 2));
    }
  }
  for (bool landscape : {false, true}) {
    const int w = landscape ? 800 : 480, h = landscape ? 480 : 800;
    const auto footer = paperFooterRect(w, h);
    assert(footer.x == 0 && footer.w == w && footer.y == h - 44 && footer.h == 44);
    assert(!footer.contains(w / 2, h - 45) && footer.contains(w - 1, h - 1));
    for (int design = 0; design < 3; ++design) {
      const auto card = paperCardLayout(design, landscape);
      for (auto box : {card.name, card.avatar, card.account, card.email, card.comment, card.qr})
        if (box.h) assert(box.y + box.h <= footer.y);
    }
  }
  assert(paperMenuRowHeight > paperMenuFontSize + 32);
  assert(paperMenuTop + 5 * paperMenuRowHeight < 800 - 44);
}
