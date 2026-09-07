#pragma once
#include "PaperFont.h"
#include "CardLayout.h"
#include "UiLanguage.h"
#include <cstdio>
#include <ctime>

namespace tc {
// Native portrait geometry. Photos keep the existing square crop and storage.
constexpr CardRect paperHomePhoto{24, 216, 432, 432};
constexpr CardRect paperHomeClock{24, 38, 432, 132};
inline constexpr PaperFont paperClockFont{128}, paperCalendarDayFont{36};
inline constexpr int calendarDays(int year, int month) {
  return month == 2 ? 28 + (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
       : (month == 4 || month == 6 || month == 9 || month == 11) ? 30 : 31;
}
inline const char *paperWeekday(int index) {
  static const char *ja[] = {"日", "月", "火", "水", "木", "金", "土"};
  static const char *en[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
  return tr(ja[index], en[index]);
}
inline void homeText(lgfx::LGFXBase &d, const char *value, int x, int y,
                     const lgfx::IFont &font, uint32_t fg = 0x000000u,
                     uint32_t bg = 0xFFFFFFu) {
  d.setFont(&font); d.setTextSize(1); d.setTextColor(fg, bg);
  d.drawString(value, x, y);
}
inline void drawPaperBattery(lgfx::LGFXBase &d, int level) {
  setPaperFontMonochrome(true);
  d.fillRect(d.width() - 96, 0, 96, 30, 0xFFFFFFu);
  char value[16] = "--%";
  if (level >= 0) snprintf(value, sizeof(value), "%d%%", level);
  d.setFont(&paper24); d.setTextSize(1);
  homeText(d, value, d.width() - 16 - d.textWidth(value), 6, paper24);
}
inline void drawPaperStatus(lgfx::LGFXBase &d, const tm &date, bool valid, int level) {
  setPaperFontMonochrome(true);
  d.fillRect(0, 0, 132, 30, 0xFFFFFFu);
  char value[16] = "--:--";
  if (valid) snprintf(value, sizeof(value), "%02d:%02d", date.tm_hour, date.tm_min);
  homeText(d, value, 16, 6, paper24);
  drawPaperBattery(d, level);
}
inline void homeCentered(lgfx::LGFXBase &d, const char *value, int center, int y,
                         const lgfx::IFont &font, bool inverted = false) {
  d.setFont(&font); d.setTextSize(1);
  homeText(d, value, center - d.textWidth(value) / 2, y, font,
           inverted ? 0xFFFFFFu : 0x000000u,
           inverted ? 0x000000u : 0xFFFFFFu);
}
inline void paperDateLabel(char *out, size_t size, const tm &date) {
  snprintf(out, size, tr("%d月%d日（%s）", "%d/%d (%s)"),
           date.tm_mon + 1, date.tm_mday, paperWeekday(date.tm_wday));
}
inline void drawPaperHomeTime(lgfx::LGFXBase &d, const tm &date, bool valid) {
  // The only minute-update rectangle is wholly above the photo. Render the
  // same monochrome glyphs on first draw and partial refresh to avoid popping.
  setPaperFontMonochrome(true);
  const auto b = paperHomeClock;
  d.fillRect(b.x, b.y, b.w, b.h, 0xFFFFFFu);
  char value[16] = "--:--";
  if (valid) snprintf(value, sizeof(value), "%02d:%02d", date.tm_hour, date.tm_min);
  homeText(d, value, b.x - 3, b.y, paperClockFont);
}
inline void drawPaperHomeDetails(lgfx::LGFXBase &d, const tm &date, bool valid) {
  setPaperFontMonochrome(true);
  char value[64];
  if (valid) snprintf(value, sizeof(value), "%d", date.tm_year + 1900);
  else snprintf(value, sizeof(value), "%s", tr("時計未設定", "Clock not set"));
  homeText(d, value, 24, 6, paper24);
  if (valid) paperDateLabel(value, sizeof(value), date);
  else snprintf(value, sizeof(value), "%s", tr("スマホから日時を設定", "Set the clock from your phone"));
  homeText(d, value, 24, 174, valid ? paper32 : paper24);
  const int previousMonth = date.tm_mon == 0 ? 12 : date.tm_mon;
  const int previousYear = date.tm_year + 1900 - (date.tm_mon == 0);
  for (int col = 0; col < 7; ++col) {
    const int cx = 24 + (2 * col + 1) * 432 / 14;
    homeCentered(d, paperWeekday(col), cx, 666, paper24);
    if (!valid) continue;
    int number = date.tm_mday + col - date.tm_wday;
    if (number < 1) number += calendarDays(previousYear, previousMonth);
    else if (number > calendarDays(date.tm_year + 1900, date.tm_mon + 1))
      number -= calendarDays(date.tm_year + 1900, date.tm_mon + 1);
    const bool today = col == date.tm_wday;
    if (today) d.fillCircle(cx, 720, 23, 0x000000u);
    snprintf(value, sizeof(value), "%d", number);
    homeCentered(d, value, cx, 703, paper28, today);
  }
}
inline void drawPaperMonth(lgfx::LGFXBase &d, const tm &date, bool valid) {
  setPaperFontMonochrome(true);
  if (!valid) {
    homeText(d, tr("カレンダー", "Calendar"), 24, 92, paper52);
    homeText(d, tr("まだ日時が設定されていません", "The clock is not set yet"), 24, 280, paper28);
    homeText(d, tr("スマホから日時を合わせてください", "Set it from your phone"), 24, 328, paper24);
    return; // Caller always draws the Home footer, even without a clock.
  }
  char value[64];
  snprintf(value, sizeof(value), "%d", date.tm_year + 1900);
  homeText(d, value, 24, 60, paper28);
  static const char *months[] = {"January", "February", "March", "April", "May", "June",
                                "July", "August", "September", "October", "November", "December"};
  if (uiLanguage == Language::English)
    homeText(d, months[date.tm_mon], 20, 106, paper52);
  else {
    snprintf(value, sizeof(value), "%d月", date.tm_mon + 1);
    homeText(d, value, 20, 90, paper96);
  }
  d.drawFastHLine(24, 202, 432, 0x000000u);
  homeText(d, tr("今日", "Today"), 24, 218, paper24);
  paperDateLabel(value, sizeof(value), date);
  d.setFont(&paper28); d.setTextSize(1);
  homeText(d, value, 456 - d.textWidth(value), 214, paper28);
  for (int col = 0; col < 7; ++col) {
    const int cx = 24 + (2 * col + 1) * 432 / 14;
    homeCentered(d, paperWeekday(col), cx, 272, paper24);
    if (col == 0 || col == 6) d.drawFastHLine(cx - 8, 305, 16, 0x000000u);
  }
  const int first = (date.tm_wday - (date.tm_mday - 1) % 7 + 7) % 7;
  const int count = calendarDays(date.tm_year + 1900, date.tm_mon + 1);
  for (int number = 1; number <= count; ++number) {
    const int slot = first + number - 1;
    const int cx = 24 + (2 * (slot % 7) + 1) * 432 / 14;
    const int y = 326 + (slot / 7) * 68;
    const bool today = number == date.tm_mday;
    if (today) d.fillCircle(cx, y + 22, 26, 0x000000u);
    snprintf(value, sizeof(value), "%d", number);
    homeCentered(d, value, cx, y, paperCalendarDayFont, today);
  }
}
// Clear and repaint only one dated region at a time. A single bounding box
// spanning date and week would also drive the photograph in monochrome mode.
inline void updatePaperDateRegion(lgfx::LGFXBase &d, CardRect b,
                                  const tm &date, bool valid, bool month) {
  d.startWrite();
  d.setClipRect(b.x, b.y, b.w, b.h);
  d.fillRect(b.x, b.y, b.w, b.h, 0xFFFFFFu);
  if (month) drawPaperMonth(d, date, valid);
  else drawPaperHomeDetails(d, date, valid);
  d.clearClipRect();
  d.endWrite();
}
inline void updatePaperDate(lgfx::LGFXBase &d, const tm &date, bool valid,
                            UiScreen screen, uint8_t home) {
  if (screen == UiScreen::Month) {
    updatePaperDateRegion(d, {0, 30, d.width(), d.height() - 74}, date, valid, true);
  } else if (screen == UiScreen::Home && home == 1) {
    updatePaperDateRegion(d, {24, 0, 300, 30}, date, valid, false);
    updatePaperDateRegion(d, {24, 174, 432, 42}, date, valid, false);
    updatePaperDateRegion(d, {24, 658, 432, 94}, date, valid, false);
  }
}
} // namespace tc
