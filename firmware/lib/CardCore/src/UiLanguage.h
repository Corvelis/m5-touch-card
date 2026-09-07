#pragma once
#include <cstdint>
#ifndef TOUCH_CARD_DEFAULT_LANGUAGE
#define TOUCH_CARD_DEFAULT_LANGUAGE 0
#endif
static_assert(TOUCH_CARD_DEFAULT_LANGUAGE == 0 || TOUCH_CARD_DEFAULT_LANGUAGE == 1,
              "TOUCH_CARD_DEFAULT_LANGUAGE must be 0 (ja) or 1 (en)");
namespace tc {
enum class Language : uint8_t { Japanese = 0, English = 1 };
constexpr Language resolveLanguage(uint8_t saved, uint8_t initial = TOUCH_CARD_DEFAULT_LANGUAGE) {
  return static_cast<Language>(saved <= 1 ? saved : initial <= 1 ? initial : 0);
}
inline Language uiLanguage = resolveLanguage(255);
inline const char *tr(const char *ja, const char *en) {
  return uiLanguage == Language::English ? en : ja;
}
} // namespace tc
