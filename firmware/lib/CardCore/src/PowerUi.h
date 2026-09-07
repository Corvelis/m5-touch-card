#pragma once
#include <cstdint>
namespace tc {
// PaperMono's red indicator is M5PM1 PWR_CFG LED_EN, not the charger.
// Match M5Unified LED_PaperMono_Class::display(): clear only bit 4 at 0x06.
// Never zero the register: its other bits control charging and power rails.
template <typename Read, typename Write>
bool turnOffPaperRedLed(Read read, Write write) {
  uint8_t before, actual;
  if (!read(0x06, before)) return false;
  const uint8_t off = uint8_t(before & ~0x10);
  if (off != before && !write(0x06, off)) return false;
  return read(0x06, actual) && actual == off;
}
// M5PM1 BTN_CFG_1/2 bit 0 only. Preserve download-mode lock, delays and
// reserved bits. Read both registers before writing and verify the result.
template <typename Read, typename Write>
bool protectPaperPowerButton(Read read, Write write) {
  uint8_t first, second, actual;
  if (!read(0x49, first) || !read(0x4a, second)) return false;
  if (!write(0x49, uint8_t(first | 1)) || !write(0x4a, uint8_t(second | 1))) return false;
  return read(0x49, actual) && actual == uint8_t(first | 1) &&
         read(0x4a, actual) && actual == uint8_t(second | 1);
}
struct ScreenBlanker {
  bool off = false, waitForRelease = false;
  uint8_t brightness = 128;
  uint8_t toggle(uint8_t current) {
    off = !off;
    waitForRelease = true;
    if (off && current) brightness = current;
    return off ? 0 : brightness;
  }
  bool acceptsInput(bool touching, bool toggled) {
    if (off || toggled) return false;
    if (waitForRelease) { if (!touching) waitForRelease = false; return false; }
    return true;
  }
};
} // namespace tc
