#include "UiLanguage.h"
#include "PowerUi.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  using namespace tc;
  assert(resolveLanguage(255) == Language(TOUCH_CARD_DEFAULT_LANGUAGE));
  assert(resolveLanguage(0, 1) == Language::Japanese);
  assert(resolveLanguage(1, 0) == Language::English);
  assert(resolveLanguage(255, 1) == Language::English);
  assert(resolveLanguage(2, 0) == Language::Japanese);
  assert(resolveLanguage(255, 255) == Language::Japanese);
  uiLanguage = Language::Japanese;
  assert(std::strcmp(tr("日本語", "English"), "日本語") == 0);
  uiLanguage = Language::English;
  assert(std::strcmp(tr("日本語", "English"), "English") == 0);

  // Every PWR_CFG combination: only the red LED bit may change. In particular,
  // retain charge enable, LDO, DCDC, BOOST and all reserved bits.
  for (unsigned initial = 0; initial < 256; ++initial) {
    uint8_t value = initial;
    int writes = 0;
    auto read = [&](uint8_t reg, uint8_t &out) { assert(reg == 0x06); out = value; return true; };
    auto write = [&](uint8_t reg, uint8_t out) { assert(reg == 0x06); ++writes; value = out; return true; };
    assert(turnOffPaperRedLed(read, write));
    assert(value == (initial & ~0x10));
    assert(writes == ((initial & 0x10) ? 1 : 0));
    const int prior = writes;
    assert(turnOffPaperRedLed(read, write));
    assert(writes == prior); // Already off: no needless write.
  }
  for (int fail = 0; fail < 3; ++fail) {
    int operation = 0, writes = 0;
    uint8_t value = 0x1f;
    assert(!turnOffPaperRedLed(
      [&](uint8_t, uint8_t &out) { out = value; return operation++ != fail; },
      [&](uint8_t, uint8_t out) { ++writes; value = out; return operation++ != fail; }));
    if (fail == 0) assert(writes == 0);
  }
  assert(!turnOffPaperRedLed(
    [](uint8_t, uint8_t &value) { value = 0x1f; return true; },
    [](uint8_t, uint8_t) { return true; })); // Acknowledged but not applied.

  // Exhaustively verify that no unrelated PMIC bit is modified, including
  // BTN_CFG_1's download-mode lock bit. Setting twice must be idempotent.
  for (unsigned a = 0; a < 256; ++a) for (unsigned b = 0; b < 256; ++b) {
    std::array<uint8_t, 256> regs{};
    regs[0x49] = a; regs[0x4a] = b;
    auto read = [&](uint8_t r, uint8_t &v) { v = regs[r]; return true; };
    auto write = [&](uint8_t r, uint8_t v) { regs[r] = v; return true; };
    assert(protectPaperPowerButton(read, write));
    assert(regs[0x49] == (a | 1) && regs[0x4a] == (b | 1));
    assert(protectPaperPowerButton(read, write));
    assert(regs[0x49] == (a | 1) && regs[0x4a] == (b | 1));
  }
  for (int fail = 0; fail < 6; ++fail) {
    int operation = 0, writes = 0;
    std::array<uint8_t, 256> regs{};
    assert(!protectPaperPowerButton(
      [&](uint8_t r, uint8_t &v) { v = regs[r]; return operation++ != fail; },
      [&](uint8_t r, uint8_t v) { ++writes; regs[r] = v; return operation++ != fail; }));
    if (fail < 2) assert(writes == 0);
  }
  // A device acknowledging a write but not retaining it must report failure.
  assert(!protectPaperPowerButton(
    [](uint8_t, uint8_t &v) { v = 0; return true; },
    [](uint8_t, uint8_t) { return true; }));

  ScreenBlanker display;
  assert(display.acceptsInput(false, false));
  assert(display.toggle(73) == 0 && display.off);
  assert(!display.acceptsInput(false, true));
  assert(!display.acceptsInput(true, false));
  assert(!display.acceptsInput(false, false));
  assert(display.toggle(0) == 73 && !display.off);
  assert(!display.acceptsInput(true, true));
  assert(!display.acceptsInput(true, false));
  assert(!display.acceptsInput(false, false));
  assert(display.acceptsInput(true, false));
  assert(display.toggle(99) == 0);
  assert(display.toggle(0) == 99);
  ScreenBlanker initiallyDark;
  assert(initiallyDark.toggle(0) == 0);
  assert(initiallyDark.toggle(0) > 0);
  // PaperMono shares the input gate; only B reaches toggle while dark.
  // A wake refresh is treated as busy until complete, then all held inputs
  // (including a touch begun while dark) must be released once.
  ScreenBlanker paper;
  for (int cycle = 0; cycle < 5; ++cycle) {
    assert(paper.toggle(64) == 0);
    for (int tick = 0; tick < 10; ++tick)
      assert(!paper.acceptsInput(tick % 2, false));
    assert(paper.toggle(0) == 64);
    assert(!paper.acceptsInput(false, true)); // Wake click / display busy.
    assert(paper.waitForRelease);
    assert(!paper.acceptsInput(true, true));
    assert(!paper.acceptsInput(true, false)); // Panel done, finger still down.
    assert(!paper.acceptsInput(false, false)); // Discard release/click event.
    assert(paper.acceptsInput(false, false));
    assert(paper.acceptsInput(true, false)); // A new touch now works.
  }
  std::cout << "language persistence resolution and power controls: PASS\n";
}
