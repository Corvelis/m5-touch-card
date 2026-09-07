#include "StackDigits.h"
#include "NfcProgressView.h"
#include <cassert>
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2) return 2;
  M5Canvas d;
  d.setColorDepth(16); d.createSprite(320, 240); d.fillScreen(0xFFFFFFu);
  d.setTextSize(1); d.setTextColor(0x000000u, 0xFFFFFFu);
  d.setFont(&tc::stackClockDigits);
  assert(d.fontHeight() <= 50);
  for (const char *time : {"--:--", "00:00", "11:11", "23:59"})
    assert(d.textWidth(time) <= 160);
  d.drawString("23:59", 144, 6);
  const tc::ProgressFonts fonts{&fonts::efontJA_16, &tc::stackDigits, &fonts::efontJA_16, 1};
  tc::TransferProgress previous{tc::TransferStage::Transferring, 100, 100};
  tc::drawNfcProgress(d, previous, false, false, false, fonts);
  for (unsigned percent : {0u, 9u, 99u, 100u}) {
    tc::TransferProgress next{tc::TransferStage::Transferring, percent, 100};
    tc::drawNfcProgress(d, next, false, false, false, fonts, &previous);
    M5Canvas expected;
    expected.setColorDepth(16); expected.createSprite(320, 240); expected.fillScreen(0xFFFFFFu);
    tc::drawNfcProgress(expected, next, false, false, false, fonts);
    for (int y = 82; y < 194; ++y) for (int x = 24; x < 296; ++x)
      assert(d.readPixel(x, y) == expected.readPixel(x, y));
    previous = next;
  }
  d.setFont(&tc::stackDigits);
  assert(d.fontHeight() <= 50 && d.textWidth("100%") <= 272);
  // Every actual character must contain ink, including the error marker.
  for (char c : std::string("!%-0123456789:")) {
    M5Canvas glyph; glyph.setColorDepth(16); glyph.createSprite(64, 64);
    glyph.fillScreen(0xFFFFFFu); glyph.setFont(&tc::stackDigits);
    glyph.setTextColor(0x000000u, 0xFFFFFFu);
    char value[] = {c, 0}; glyph.drawString(value, 0, 0);
    bool ink = false;
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x)
      ink |= glyph.readPixel(x, y) != 0xFFFFu;
    assert(ink);
  }
  size_t length = 0;
  void *png = d.createPng(&length, 0, 0, d.width(), d.height());
  assert(png);
  std::ofstream(argv[1], std::ios::binary).write(static_cast<char *>(png), length);
  free(png);
  std::cout << "Stack digits: clock bounds, glyph coverage and partial progress redraw PASS\n";
}
