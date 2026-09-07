#include "DisplayPolicy.h"
#include <cassert>
#include <initializer_list>

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
