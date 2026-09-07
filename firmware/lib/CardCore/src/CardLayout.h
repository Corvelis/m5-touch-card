#pragma once
#include <cstdint>

namespace tc {
enum class UiScreen {
  Home, Menu, Exchange, Book, Card, CardActions, Delete, Text, Qr,
  HomeChoice, Settings, Design, Orientation, Preview, Storage,
  Internal, Sd, Usage, Nfc, Message, Month, Format, MutualChoice,
  SystemSettings, Language, Power, PowerOff, Restart
};

inline bool landscapeCard(bool paperMono, bool landscape, UiScreen screen,
                          uint8_t home) {
  return paperMono && landscape &&
         ((screen == UiScreen::Home && home == 0) ||
          screen == UiScreen::Card || screen == UiScreen::Preview ||
          screen == UiScreen::Qr);
}

struct CardRect {
  int x, y, w, h;
  bool contains(int px, int py) const {
    return w > 0 && h > 0 && px >= x && py >= y && px < x + w && py < y + h;
  }
};
struct LandscapeCardLayout {
  CardRect name, avatar, account, email, comment, qr, accent, band;
  int nameSize;
};

struct PaperCardLayout {
  CardRect name, avatar, account, email, comment, qr, accent, band;
  int namePx;
  bool centered;
};
inline PaperCardLayout paperCardLayout(uint8_t design, bool horizontal) {
  if (horizontal) {
    if (design == 2)
      return {{264, 102, 312, 116}, {32, 116, 208, 208},
              {264, 230, 312, 40}, {264, 282, 312, 68},
              {264, 364, 504, 40}, {600, 136, 168, 168},
              {}, {}, 52, false};
    return {{32, 84, 520, 124}, {580, 64, 188, 188},
            {32, 260, 516, 40}, {32, 306, 516, 68},
            {32, 386, 516, 36}, {600, 264, 168, 168},
            design == 0 ? CardRect{32, 230, 80, 4} : CardRect{},
            design == 1 ? CardRect{16, 48, 768, 208} : CardRect{}, 56, false};
  }
  if (design == 2)
    return {{32, 310, 416, 116}, {132, 64, 216, 216},
            {32, 434, 416, 40}, {32, 482, 416, 64},
            {32, 550, 416, 36}, {164, 594, 152, 152},
            {}, {}, 52, true};
  return {{32, 88, 208, 160}, {256, 80, 192, 192},
          {32, 310, 416, 40}, {32, 378, 416, 76},
          {254, 550, 194, 144}, {32, 518, 192, 192},
          design == 0 ? CardRect{32, 280, 80, 4} : CardRect{},
          design == 1 ? CardRect{16, 56, 448, 310} : CardRect{}, 52, false};
}

// PaperMono's native landscape canvas is 800x480. Status ends at y=30;
// the footer starts at y=436. Name/optional fields never share the QR column.
inline LandscapeCardLayout landscapeLayout(uint8_t design) {
  if (design == 2) {
    return {{200, 120, 368, 80}, {40, 180, 128, 128},
            {200, 220, 368, 20}, {200, 254, 368, 40},
            {200, 320, 368, 60}, {608, 172, 168, 168},
            {0, 0, 0, 0}, {0, 0, 0, 0}, 2};
  }
  return {{40, 80, 528, 120}, {632, 76, 128, 128},
          {40, 262, 528, 20}, {40, 294, 528, 40},
          {40, 354, 528, 40}, {612, 246, 168, 168},
          design == 1 ? CardRect{0, 0, 0, 0} : CardRect{40, 236, 80, 4},
          design == 1 ? CardRect{16, 44, 768, 180} : CardRect{0, 0, 0, 0},
          design == 1 ? 2 : 3};
}
} // namespace tc
