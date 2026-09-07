#include "PaperCard.h"
#include "PaperHome.h"
#include "PaperFooter.h"
#include "CircleCrop.h"
#include "DisplayPolicy.h"
#include "NfcProgressView.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cassert>

std::vector<unsigned char> read(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), {}};
}
void save(M5Canvas &d, const std::string &path) {
  for (int y = 0; y < d.height(); ++y) for (int x = 0; x < d.width(); ++x) {
    auto rgb = d.readPixelRGB(x, y);
    int gray = (rgb.R8() * 77 + rgb.G8() * 150 + rgb.B8() * 29) >> 8;
    gray = std::min(3, (gray + 42) / 85) * 85;
    d.drawPixel(x, y, uint32_t(gray * 0x010101));
  }
  size_t size = 0;
  void *png = d.createPng(&size, 0, 0, d.width(), d.height());
  if (!png) throw std::runtime_error("PNG encoding failed");
  std::ofstream(path, std::ios::binary).write(static_cast<char *>(png), size);
  free(png);
  std::cout << path << '\n';
}
void chrome(M5Canvas &d, bool card) {
  d.setFont(&tc::paper24); d.setTextSize(1); d.setTextColor(0x000000, 0xFFFFFF);
  d.drawString("15:50", 16, 6);
  d.drawString("82%", d.width() - 16 - d.textWidth("82%"), 6);
  if (!card) return;
  d.drawFastHLine(16, d.height() - 44, d.width() - 32, 0x000000);
  d.drawString(tc::tr("名刺交換", "Card exchange"), 20, d.height() - 34);
  d.drawString(tc::tr("メニュー", "Menu"), d.width() - 20 - d.textWidth(tc::tr("メニュー", "Menu")), d.height() - 34);
}
void verifySleepingFooter(M5Canvas &d, tc::UiScreen screen, uint8_t home) {
  const int w = d.width(), h = d.height();
  std::vector<uint16_t> before(w * h);
  for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
    before[y * w + x] = d.readPixel(x, y);
  tc::PaperFooterCache cache;
  assert(!cache.restore(d)); // Never replay an uninitialized footer.
  cache.capture(d, screen, home);
  tc::clearSleepingPaperFooter(d, false, screen, home);
  for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
    assert(d.readPixel(x, y) == before[y * w + x]);
  tc::clearSleepingPaperFooter(d, true, screen, home);
  const auto band = tc::paperFooterRect(w, h);
  for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
    const bool cleared = tc::hidePaperFooter(true, true, screen, home) && band.contains(x, y);
    assert(d.readPixel(x, y) == (cleared ? 0xFFFFu : before[y * w + x]));
  }
  const bool controls = tc::hidePaperFooter(true, true, screen, home);
  assert(cache.restore(d) == controls);
  for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
    assert(d.readPixel(x, y) == before[y * w + x]);
  // Simulate a screen change while sleeping: the newly rendered footer wins.
  if (controls) {
    d.fillRect(0, h - 44, w, 44, 0xFFFFFFu);
    d.fillRect(20, h - 30, 40, 12, 0x000000u);
    cache.capture(d, tc::UiScreen::Card, home);
    tc::clearSleepingPaperFooter(d, true, screen, home);
    assert(cache.restore(d));
    assert(d.readPixel(25, h - 25) == 0);
    assert(d.readPixel(w - 25, h - 25) == 0xFFFFu);
    // Full-screen photo invalidates the previous card's cached controls.
    cache.capture(d, tc::UiScreen::Home, 2);
    assert(!cache.restore(d));
  }
  tc::clearSleepingPaperFooter(d, true, screen, home);
}
int main(int argc, char **argv) {
  if (argc != 4 && argc != 5) return 2;
  if (argc == 5) tc::uiLanguage = std::string(argv[4]) == "en" ? tc::Language::English : tc::Language::Japanese;
  const auto font = read(argv[1]), photo = read(argv[2]);
  if (font.empty() || photo.empty() || !tc::initPaperFont(font.data())) return 3;
  for (bool landscape : {false, true}) {
    const int w = landscape ? 800 : 480, h = landscape ? 480 : 800;
    M5Canvas d, expected;
    for (auto *p : {&d, &expected}) {
      p->setColorDepth(16); p->createSprite(w, h); p->fillScreen(0x555555u);
    }
    tm date{}; date.tm_hour = 23; date.tm_min = 59;
    tc::drawPaperStatus(d, date, true, 100);
    tc::PaperFooterCache cache;
    cache.capture(d, tc::UiScreen::Card, 0);
    tc::clearSleepingPaperFooter(d, true, tc::UiScreen::Card, 0);
    std::vector<uint16_t> before(w * h);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
      before[y * w + x] = d.readPixel(x, y);
    date.tm_hour = date.tm_min = 0;
    for (int level : {99, 9, 0, -1}) {
      tc::drawPaperStatus(d, date, level != -1, level);
      tc::drawPaperStatus(expected, date, level != -1, level);
      for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        const bool status = y < 30 && (x < 132 || x >= w - 96);
        assert(d.readPixel(x, y) == (status ? expected.readPixel(x, y) : before[y * w + x]));
      }
    }
    assert(cache.restore(d));
    // Waking must retain the newest sleeping clock/battery, not the old values.
    for (int y = 0; y < 30; ++y) for (int x = 0; x < w; ++x)
      assert(d.readPixel(x, y) == expected.readPixel(x, y));
    M5Canvas rotated; rotated.setColorDepth(16); rotated.createSprite(h, w);
    assert(!cache.restore(rotated));
  }
  std::cout << "Sleeping minute status / footer-only wake checks: PASS\n";
  std::filesystem::create_directories(argv[3]);
  const std::string out = argv[3];
  // Full-screen photo pixels must remain untouched, including the bottom band.
  M5Canvas fullscreen;
  fullscreen.setColorDepth(16); fullscreen.createSprite(480, 800);
  fullscreen.fillScreen(0x555555u);
  verifySleepingFooter(fullscreen, tc::UiScreen::Home, 2);
  // The dashboard and month view use exactly the firmware renderer, with a
  // square crop matching the existing phone image slot. No HTML recreation.
  for (bool valid : {true, false}) {
    tm date{}; date.tm_year = 126; date.tm_mon = 8; date.tm_mday = 6;
    date.tm_wday = 0; date.tm_hour = 15; date.tm_min = 50;
    M5Canvas home, month;
    for (auto *d : {&home, &month}) {
      d->setColorDepth(16); d->createSprite(480, 800); d->fillScreen(0xFFFFFFu);
    }
    const auto box = tc::paperHomePhoto;
    home.drawJpg(photo.data(), photo.size(), box.x, box.y, 0, 0, 0, 0, box.w / 386.0f);
    tc::drawPaperHomeDetails(home, date, valid);
    tc::drawPaperHomeTime(home, date, valid);
    tc::homeText(home, "82%", 408, 6, tc::paper24);
    home.drawFastHLine(16, 756, 448, 0x000000u);
    tc::homeText(home, tc::tr("カレンダー", "Calendar"), 20, 766, tc::paper24);
    home.setFont(&tc::paper24);
    tc::homeText(home, tc::tr("メニュー", "Menu"), 460 - home.textWidth(tc::tr("メニュー", "Menu")), 766, tc::paper24);
    save(home, out + (valid ? "/home.png" : "/home-unset.png"));
    chrome(month, false);
    tc::drawPaperMonth(month, date, valid);
    month.drawFastHLine(16, 756, 448, 0x000000u);
    tc::homeText(month, tc::tr("ホーム", "Home"), 20, 766, tc::paper24);
    save(month, out + (valid ? "/month.png" : "/month-unset.png"));
    if (valid) for (int y = 0; y < 30; ++y) for (int x = 24; x < 456; ++x)
      assert(home.readPixel(x, 666 + y) == month.readPixel(x, 272 + y));
    // Digit width changes must fully erase the old clock and never touch the
    // photo, date, week or footer. Compare against a clean clock render.
    M5Canvas reference;
    reference.setColorDepth(16); reference.createSprite(480, 800);
    reference.fillScreen(0xFFFFFFu);
    std::vector<uint32_t> original(480 * 800);
    for (int y = 0; y < 800; ++y) for (int x = 0; x < 480; ++x)
      original[y * 480 + x] = home.readPixel(x, y);
    for (int hour : {0, 8, 11, 23}) {
      date.tm_hour = hour; date.tm_min = 1;
      tc::drawPaperHomeTime(home, date, valid);
      tc::drawPaperHomeTime(reference, date, valid);
      const auto b = tc::paperHomeClock;
      for (int y = 0; y < 800; ++y) for (int x = 0; x < 480; ++x) {
        const bool inside = x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
        if (home.readPixel(x, y) != (inside ? reference.readPixel(x, y) : original[y * 480 + x]))
          throw std::runtime_error("Home clock update changed unrelated pixels or left old digits");
      }
    }
    verifySleepingFooter(home, tc::UiScreen::Home, 1);
    if (valid) save(home, out + "/home-light-off.png");
    // Midnight catch-up: compare date/week with a clean draw and ensure the
    // photo, large clock, battery and hidden footer remain byte-for-byte intact.
    for (int y = 0; y < 800; ++y) for (int x = 0; x < 480; ++x)
      original[y * 480 + x] = home.readPixel(x, y);
    date.tm_year = 127; date.tm_mon = 0; date.tm_mday = 1; date.tm_wday = 5;
    reference.fillScreen(0xFFFFFFu);
    tc::drawPaperHomeDetails(reference, date, true);
    tc::updatePaperDate(home, date, true, tc::UiScreen::Home, 1);
    for (int y = 0; y < 800; ++y) for (int x = 0; x < 480; ++x) {
      bool changed = tc::CardRect{24, 0, 300, 30}.contains(x, y) ||
                     tc::CardRect{24, 174, 432, 42}.contains(x, y) ||
                     tc::CardRect{24, 658, 432, 94}.contains(x, y);
      assert(home.readPixel(x, y) == (changed ? reference.readPixel(x, y) : original[y * 480 + x]));
    }
    for (int y = 0; y < 800; ++y) for (int x = 0; x < 480; ++x)
      original[y * 480 + x] = month.readPixel(x, y);
    reference.fillScreen(0xFFFFFFu); tc::drawPaperMonth(reference, date, true);
    tc::updatePaperDate(month, date, true, tc::UiScreen::Month, 0);
    for (int y = 0; y < 800; ++y) for (int x = 0; x < 480; ++x)
      assert(month.readPixel(x, y) == (y >= 30 && y < 756 ? reference.readPixel(x, y) : original[y * 480 + x]));
  }
  assert(tc::calendarDays(2024, 2) == 29 && tc::calendarDays(2100, 2) == 28);
  assert(tc::calendarDays(2000, 2) == 29 && tc::calendarDays(2026, 9) == 30);
  for (int monthIndex = 0; monthIndex < 12; ++monthIndex) {
    tm date{}; date.tm_year = 124; date.tm_mon = monthIndex;
    date.tm_mday = tc::calendarDays(2024, monthIndex + 1); date.tm_hour = 12;
    date.tm_isdst = -1; mktime(&date);
    M5Canvas month; month.setColorDepth(16); month.createSprite(480, 800);
    month.fillScreen(0xFFFFFFu); tc::drawPaperMonth(month, date, true);
    // Every month, including leap February and six-row months, stays clear
    // of both the status strip and the footer touch region.
    for (int y = 0; y < 800; ++y) if (y < 30 || y >= 750)
      for (int x = 0; x < 480; ++x) assert(month.readPixel(x, y) == 0xFFFFu);
    if (monthIndex == 1 || monthIndex == 5)
      save(month, out + "/month-2024-" + std::to_string(monthIndex + 1) + ".png");
  }
  std::cout << "Home partial refresh / calendar checks: PASS\n";
  // Differential rendering must look exactly like a full render, including
  // digit-width changes and a lower offset after a peer resumes a transfer.
  for (bool paper : {true, false}) {
    tc::setPaperFontMonochrome(true);
    const tc::ProgressFonts pf = paper
        ? tc::ProgressFonts{&tc::paper32, &tc::paper96, &tc::paper24, 1}
        : tc::ProgressFonts{&fonts::efontJA_16, &fonts::FreeSans24pt7b, &fonts::efontJA_16, 1};
    for (bool sender : {true, false}) {
      M5Canvas incremental, reference;
      for (auto *canvas : {&incremental, &reference}) {
        canvas->setColorDepth(16);
        canvas->createSprite(paper ? 480 : 320, paper ? 800 : 240);
        canvas->fillScreen(0xFFFFFF);
      }
      tc::TransferProgress previous{tc::TransferStage::Transferring, 0, 14815};
      tc::drawNfcProgress(incremental, previous, paper, sender, false, pf);
      for (unsigned offset : {148U, 1481U, 14666U, 14815U, 2963U}) {
        tc::TransferProgress next{tc::TransferStage::Transferring, offset, 14815};
        tc::drawNfcProgress(incremental, next, paper, sender, false, pf, &previous);
        tc::drawNfcProgress(reference, next, paper, sender, false, pf);
        for (int y = 0; y < reference.height(); ++y)
          for (int x = 0; x < reference.width(); ++x)
            if (incremental.readPixel(x, y) != reference.readPixel(x, y)) {
              std::cerr << "paper=" << paper << " offset=" << offset << " pixel=" << x << ',' << y << '\n';
              throw std::runtime_error("Partial progress rendering differs from full rendering");
            }
        previous = next;
      }
    }
  }
  std::cout << "Partial progress pixel equivalence: PASS\n";
  for (bool paper : {true, false}) {
    const tc::ProgressFonts pf = paper
        ? tc::ProgressFonts{&tc::paper32, &tc::paper96, &tc::paper24, 1}
        : tc::ProgressFonts{&fonts::efontJA_16, &fonts::FreeSans24pt7b, &fonts::efontJA_16, 1};
    for (int leg : {1, 2}) {
      M5Canvas d; d.setColorDepth(16); d.createSprite(paper ? 480 : 320, paper ? 800 : 240);
      d.fillScreen(0xFFFFFF); tc::setPaperFontMonochrome(true);
      d.setFont(pf.label); d.setTextSize(1); d.setTextColor(0x000000, 0xFFFFFF);
      d.drawString(leg == 1 ? tc::tr("交換 1/2・渡す", "Exchange 1/2 - Send") : tc::tr("交換 2/2・受け取る", "Exchange 2/2 - Receive"), 20, 42);
      if (!paper) { d.setFont(pf.detail); d.drawString(tc::tr("2枚目の完了まで離さないでください", "Keep touching for both cards"), 24, 62); }
      else { d.setFont(pf.detail); d.drawString(tc::tr("2枚目まで自動で交換します", "Both cards exchange automatically"), 32, 128); }
      tc::TransferProgress p{leg == 1 ? tc::TransferStage::Complete : tc::TransferStage::Transferring,
                             leg == 1 ? 6000U : 2400U, 6000};
      tc::drawNfcProgress(d, p, paper, leg == 1, false, pf, nullptr, leg == 1);
      d.setFont(pf.detail); d.setTextSize(1);
      d.drawFastHLine(16, d.height() - 44, d.width() - 32, 0x000000);
      d.drawString(tc::tr("キャンセル", "Cancel"), 20, d.height() - (paper ? 34 : 30));
      save(d, out + (paper ? "/mutual-paper-" : "/mutual-stack-") + std::to_string(leg) + ".png");
    }
  }
  for (bool paper : {true, false}) {
    int index = 0;
    for (auto stage : {tc::TransferStage::Waiting, tc::TransferStage::Connecting,
         tc::TransferStage::Transferring, tc::TransferStage::Reconnecting,
         tc::TransferStage::Saving, tc::TransferStage::Complete, tc::TransferStage::Failed}) {
      M5Canvas d; d.setColorDepth(16); d.createSprite(paper ? 480 : 320, paper ? 800 : 240);
      d.fillScreen(0xFFFFFF); tc::setPaperFontMonochrome(true);
      const tc::ProgressFonts fonts = paper
          ? tc::ProgressFonts{&tc::paper32, &tc::paper96, &tc::paper24, 1}
          : tc::ProgressFonts{&fonts::efontJA_16, &fonts::FreeSans24pt7b, &fonts::efontJA_16, 1};
      d.setFont(fonts.detail); d.setTextSize(1); d.setTextColor(0x000000, 0xFFFFFF);
      d.drawString("15:50", 16, 6); d.drawString("82%", d.width() - 65, 6);
      d.setFont(fonts.label); d.drawString(tc::tr("名刺を渡す", "Send card"), 20, 42);
      d.setFont(fonts.detail);
      d.drawString(paper ? "相手を「受け取る」にしてください" : tc::tr("完了まで離さないでください", "Keep touching until done."), paper ? 32 : 24, paper ? 128 : 62);
      tc::TransferProgress progress{stage, 5926, 14815};
      if (stage == tc::TransferStage::Waiting || stage == tc::TransferStage::Connecting)
        progress.transferred = progress.total = 0;
      if (stage == tc::TransferStage::Saving || stage == tc::TransferStage::Complete)
        progress.transferred = progress.total;
      tc::drawNfcProgress(d, progress, paper, true, false, fonts);
      d.setFont(fonts.detail); d.setTextSize(1);
      d.drawFastHLine(16, d.height() - 44, d.width() - 32, 0x000000);
      d.drawString(tc::tr("キャンセル", "Cancel"), 20, d.height() - (paper ? 34 : 30));
      save(d, out + (paper ? "/nfc-paper-" : "/nfc-stack-") + std::to_string(index++) + ".png");
    }
  }
  for (bool longText : {false, true})
  for (bool horizontal : {false, true}) for (int design = 0; design < 3; ++design) {
    M5Canvas d;
    d.setColorDepth(16); d.createSprite(horizontal ? 800 : 480, horizontal ? 480 : 800);
    d.fillScreen(0xFFFFFF);
    tc::PaperProfile p{"Haru Aoki", "@haru_studio", "hello@example.com",
                      "つくる、つながる。", "https://example.com/haru"};
    if (longText)
      p = {"山田 太郎・ものづくり研究室", "@creative_developer_studio",
           "very.long.address@example.com", "電子工作とデザインで、新しいつながりを。",
           "https://example.com/cards/creative-developer-studio"};
    tc::drawPaperCard(d, p, design, horizontal, [&](tc::CardRect b, uint32_t bg) {
      d.drawJpg(photo.data(), photo.size(), b.x, b.y, 0, 0, 0, 0, b.w / 386.0f);
      for (int y = 0; y < b.h; ++y) {
        const int inset = tc::circleInset(b.w, y);
        if (inset) {
          d.drawFastHLine(b.x, b.y + y, inset, bg);
          d.drawFastHLine(b.x + b.w - inset, b.y + y, inset, bg);
        }
      }
    });
    chrome(d, true);
    save(d, out + (longText ? "/long-" : "/card-") +
         (horizontal ? "landscape-" : "portrait-") + std::to_string(design) + ".png");
    verifySleepingFooter(d, tc::UiScreen::Card, 0);
    if (!longText) save(d, out + "/light-off-" +
        (horizontal ? "landscape-" : "portrait-") + std::to_string(design) + ".png");
  }
  for (bool settings : {false, true}) {
    tc::setPaperFontMonochrome(true);
    M5Canvas d; d.setColorDepth(16); d.createSprite(480, 800); d.fillScreen(0xFFFFFF);
    chrome(d, false);
    d.setFont(&tc::paper32); d.drawString(settings ? tc::tr("設定", "Settings") : tc::tr("メニュー", "Menu"), 24, 42);
    const std::vector<const char *> labels = settings ?
      std::vector<const char *>{tc::tr("名刺デザイン", "Card design"), tc::tr("名刺の向き：縦", "Orientation: portrait"), tc::tr("ストレージ", "Storage"), tc::tr("スマホから更新", "Phone update"), tc::tr("その他", "More")} :
      std::vector<const char *>{tc::tr("名刺交換", "Card exchange"), tc::tr("名刺帳", "Cards"), tc::tr("ホーム表示", "Home screen"), tc::tr("設定", "Settings")};
    for (size_t i = 0; i < labels.size(); ++i)
      tc::drawPaperMenuRow(d, labels[i], tc::paperMenuTop + i * tc::paperMenuRowHeight,
                           tc::paperMenuRowHeight, i == 0);
    {
      d.drawFastHLine(16, 756, 448, 0x000000u);
      d.setFont(&tc::paper24);
      d.drawString(settings ? tc::tr("戻る", "Back") : tc::tr("ホームへ戻る", "Home"), 20, 766);
    }
    save(d, out + (settings ? "/settings.png" : "/menu.png"));
  }
  tc::setPaperFontMonochrome(false);
  M5Canvas book; book.setColorDepth(16); book.createSprite(480, 800); book.fillScreen(0xFFFFFF);
  chrome(book, false); book.setFont(&tc::paper32);
  book.drawString(tc::tr("名刺帳・最新順", "Cards: newest"), 20, 42);
  book.drawRoundRect(350, 38, 110, 44, 5, 0x000000u);
  book.setFont(&tc::paper24); book.drawString(tc::tr("戻る", "Back"), 381, 48);
  for (int i = 0; i < 5; ++i) {
    tc::drawPaperBookEntry(book, i % 2 ? "山田 太郎" : "Haru Aoki",
                           i % 2 ? "@yamada_studio" : "@haru_studio", 100 + i * 120,
                           i == 0, [&](tc::CardRect b) {
      book.drawJpg(photo.data(), photo.size(), b.x, b.y, 0, 0, 0, 0, b.w / 386.0f);
      for (int y = 0; y < b.h; ++y) {
        const int inset = tc::circleInset(b.w, y);
        if (inset) {
          book.drawFastHLine(b.x, b.y + y, inset, 0xFFFFFFu);
          book.drawFastHLine(b.x + b.w - inset, b.y + y, inset, 0xFFFFFFu);
        }
      }
    });
  }
  book.setFont(&tc::paper24); book.setTextColor(0x000000, 0xFFFFFF);
  book.drawFastHLine(16, 756, 448, 0x000000u);
  book.drawString(tc::tr("並び順", "Sort"), 20, 766); book.drawString(tc::tr("前へ", "Previous"), 216, 766);
  book.drawString(tc::tr("次へ", "Next"), 412, 766);
  save(book, out + "/book.png");
  book.setFont(&tc::paper52); book.setTextSize(1);
  const auto words = tc::paperLines(book, "Haru Aoki", 208);
  assert(words.size() == 2 && words[0] == "Haru" && words[1] == "Aoki");
}
