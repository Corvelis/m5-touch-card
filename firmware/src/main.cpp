#include "CardNfc.h"
#include "CardLayout.h"
#include "CircleCrop.h"
#include "DisplayPolicy.h"
#include "NfcProgressView.h"
#include "TouchLayout.h"
#include <UiLanguage.h>
#include <PowerUi.h>
#if TOUCH_CARD_STACKCHAN
#include "StackDigits.h"
#endif
#if TOUCH_CARD_PAPER_MONO
#include "PaperCard.h"
#include "PaperHome.h"
#include "PaperFooter.h"
extern const unsigned char paperFontData[] asm("_binary_assets_TouchSansJP_ttf_start");
#endif
#include "DefaultImageData.h"
#include <Arduino.h>
#include <M5Unified.h>
#include <Preferences.h>

namespace {
DeviceStorage storage;
CardService cards(storage);
RtcClock clockValue;
CardNfc nfc(storage, cards, clockValue);
Preferences settings;
bool prefs = false;
using Screen = tc::UiScreen;
Screen screen = Screen::Home, detailReturn = Screen::Home;
uint8_t design = 0, home = 0;
int focus = 0, textPage = 0, usageMedium = 0;
bool nameSort = false;
bool landscape = false;
uint8_t deleteMask = 0;
uint64_t deleteInternalGeneration = 0, deleteSdGeneration = 0;
String message, selectedId;
JsonDocument shown;
std::vector<String> labels;
int rowTop = 70, rowHeight = 50;
uint32_t lastClock = 0;
uint32_t lastStatusUpdate = 0;
int lastDay = -1;
int lastMinute = -1;
bool displayInitialized = false;
bool nfcUiSender = false, nfcUiPhone = false;
tc::ProgressRefresh nfcProgressRefresh;
tc::MutualExchange mutual;
tc::ScreenBlanker screenBlanker;
#if TOUCH_CARD_PAPER_MONO
tc::PaperFooterCache paperFooterCache;
#endif
int dashboardBattery = -999;
bool paper() {
#if TOUCH_CARD_PAPER_MONO
  return true;
#else
  return false;
#endif
}
bool landscapeCard() {
  return tc::landscapeCard(paper(), landscape, screen, home);
}
const lgfx::IFont *font16() {
#if TOUCH_CARD_PAPER_MONO
  return &tc::paper16;
#else
  return &fonts::efontJA_16;
#endif
}
const lgfx::IFont *font24() {
#if TOUCH_CARD_PAPER_MONO
  return &tc::paper24;
#else
  return &fonts::efontJA_16;
#endif
}
void text(const String &s, int x, int y, int size = 1,
          uint32_t foreground = TFT_BLACK, uint32_t background = TFT_WHITE) {
  auto &d = M5.Display;
  // TFT_* constants are RGB565; widening 0xFFFF to uint32_t turns white cyan.
  d.setTextColor(uint16_t(foreground), uint16_t(background));
  d.setTextSize(size);
  d.drawString(s, x, y);
}
String bytes(uint64_t n) {
  char b[40];
  if (n >= 1073741824ULL)
    snprintf(b, sizeof(b), "%.2f GiB", n / 1073741824.0);
  else if (n >= 1048576)
    snprintf(b, sizeof(b), "%.1f MiB", n / 1048576.0);
  else
    snprintf(b, sizeof(b), "%llu KiB", (unsigned long long)((n + 1023) / 1024));
  return b;
}
void heading(const String &value, int x = 24) {
  auto &d = M5.Display;
  const auto *font = d.getFont();
  d.setFont(font16());
  text(value, x, 42, paper() ? 2 : 1);
  d.setFont(font);
}
std::vector<String> lines(const String &s, int width, int size = 1) {
  M5.Display.setTextSize(size);
  std::vector<String> result;
  String line;
  for (unsigned i = 0; i < s.length();) {
    uint8_t c = s[i];
    unsigned length = c < 128 ? 1 : c < 224 ? 2 : c < 240 ? 3 : 4;
    String next = s.substring(i, std::min(unsigned(s.length()), i + length));
    i += length;
    if (next == "\n") {
      result.push_back(line);
      line = "";
      continue;
    }
    if (M5.Display.textWidth(line + next) > width && !line.isEmpty()) {
      result.push_back(line);
      line = "";
    }
    line += next;
  }
  if (!line.isEmpty())
    result.push_back(line);
  return result;
}
void wrap(const String &s, int x, int y, int width, int maximum, int size = 1,
          uint32_t fg = TFT_BLACK, uint32_t bg = TFT_WHITE) {
  auto list = lines(s, width, size);
  for (int i = 0; i < std::min(maximum, int(list.size())); i++) {
    String value = list[i];
    if (i == maximum - 1 && int(list.size()) > maximum) {
      while (value.length() && M5.Display.textWidth(value + "…") > width) {
        int last = value.length() - 1;
        while (last > 0 && (uint8_t(value[last]) & 0xc0) == 0x80)
          --last;
        value.remove(last);
      }
      value += "…";
    }
    const int step = std::max(20 * size, int(M5.Display.fontHeight()) + 4 * size);
    text(value, x, y + i * step, size, fg, bg);
  }
}
void status() {
  auto &d = M5.Display;
  const auto *font = d.getFont();
#if TOUCH_CARD_PAPER_MONO
  const auto now = clockValue.localTime();
  tc::drawPaperStatus(d, now.value, now.valid, M5.Power.getBatteryLevel());
  d.setFont(font);
  return;
#else
  if (paper())
    d.setFont(font24());
  d.fillRect(0, 0, d.width(), 30, TFT_WHITE);
  char b[16] = "--:--";
  auto now = clockValue.localTime();
  if (now.valid)
    strftime(b, sizeof(b), "%H:%M", &now.value);
  text(b, 16, 6);
  int level = M5.Power.getBatteryLevel();
  String battery = level < 0 ? "--%" : String(level) + "%";
  text(battery, d.width() - 16 - d.textWidth(battery), 6);
  d.setFont(font);
#endif
}
void photo(JsonVariantConst image, int x, int y, int w, int h) {
  tc::Bytes decoded;
  const uint8_t *data = kPaperMonoDefaultImage;
  size_t length = kPaperMonoDefaultImageSize;
  int iw = 386, ih = 386;
  if (!image.isNull() && decodeImage(image, decoded)) {
    data = decoded.data();
    length = decoded.size();
    iw = image["width"];
    ih = image["height"];
  }
  float scale = std::max(float(w) / iw, float(h) / ih);
  auto &d = M5.Display;
  d.setClipRect(x, y, w, h);
  d.drawJpg(data, length, x + (w - int(iw * scale)) / 2,
            y + (h - int(ih * scale)) / 2, 0, 0, 0, 0, scale);
  d.clearClipRect();
}
void avatarPhoto(JsonVariantConst image, int x, int y, int size,
                 uint32_t background = TFT_WHITE) {
  if (background == uint32_t(TFT_WHITE)) background = 0xFFFFFFu;
  photo(image, x, y, size, size);
  auto &d = M5.Display;
  for (int row = 0; row < size; ++row) {
    int inset = tc::circleInset(size, row);
    if (inset > 0) {
      d.drawFastHLine(x, y + row, inset, background);
      d.drawFastHLine(x + size - inset, y + row, inset, background);
    }
  }
}
void footer(const String &left, const String &right = tc::tr("戻る", "Back")) {
  auto &d = M5.Display;
  const auto *font = d.getFont();
  if (paper())
    d.setFont(font24());
  d.setTextSize(1);
  int y = d.height() - 44;
  d.drawFastHLine(16, y, d.width() - 32, TFT_BLACK);
  text(left, 20, y + (paper() ? 10 : 14));
  if (!right.isEmpty())
    text(right, d.width() - 20 - d.textWidth(right), y + (paper() ? 10 : 14));
  d.setFont(font);
}
void transferProgress(bool fullRender = false) {
  auto &d = M5.Display;
  const auto p = mutual.switching ? tc::TransferProgress{tc::TransferStage::Connecting, 0, 0}
                                 : nfc.progress(millis());
  if (!fullRender && !nfcProgressRefresh.needed(p, millis(), paper(),
                                               paper() && d.displayBusy())) return;
  const uint32_t paintStarted = micros();
  const auto *font = d.getFont();
#if TOUCH_CARD_PAPER_MONO
  const tc::ProgressFonts fonts{&tc::paper32, &tc::paper96, &tc::paper24, 1};
  tc::setPaperFontMonochrome(true);
  if (!fullRender) d.setEpdMode(epd_mode_t::epd_fastest);
#else
  const tc::ProgressFonts fonts{font16(), &tc::stackDigits, font16(), 1};
#endif
  if (!fullRender) d.startWrite();
  tc::drawNfcProgress(d, p, paper(), nfcUiSender, nfcUiPhone, fonts,
      !fullRender && nfcProgressRefresh.initialized ? &nfcProgressRefresh.shown : nullptr,
      mutual.active && mutual.leg == 1);
  if (!fullRender) d.endWrite();
  d.setFont(font); d.setTextSize(1);
  nfcProgressRefresh.record(p, millis());
  if (!fullRender) nfc.recordPaint(micros() - paintStarted);
}
void rows(std::initializer_list<String> entries) {
  labels.assign(entries);
  rowTop = paper() ? tc::paperMenuTop : 64;
  rowHeight = std::min(paper() ? tc::paperMenuRowHeight : 78, (M5.Display.height() - rowTop - 42) /
                               std::max(1, int(labels.size())));
  const auto *font = M5.Display.getFont();
  M5.Display.setFont(font16());
  for (size_t i = 0; i < labels.size(); i++) {
    int y = rowTop + i * rowHeight;
#if TOUCH_CARD_PAPER_MONO
    tc::drawPaperMenuRow(M5.Display, labels[i].c_str(), y, rowHeight, int(i) == focus);
#else
    if (int(i) == focus)
      M5.Display.drawRoundRect(10, y, M5.Display.width() - 20, rowHeight - 2, 5,
                               TFT_BLACK);
    wrap(labels[i], 24, y + (rowHeight - (paper() ? tc::paperMenuFontSize : 16)) / 2,
         M5.Display.width() - 48, 1, paper() ? 2 : 1);
#endif
  }
  M5.Display.setFont(font);
}
void moveFocus() {
  if (labels.size() < 2)
    return;
  const int previous = focus;
  focus = (focus + 1) % labels.size();
  auto &d = M5.Display;
#if TOUCH_CARD_PAPER_MONO
  d.setEpdMode(epd_mode_t::epd_fastest);
#endif
  d.startWrite();
  for (int row : {previous, focus}) {
    const int y = rowTop + row * rowHeight;
    const auto color = row == focus ? TFT_BLACK : TFT_WHITE;
    if (screen == Screen::Book) {
#if TOUCH_CARD_PAPER_MONO
      d.fillRoundRect(16, y + 24, 4, rowHeight - 48, 2, color);
#else
      d.fillRect(16, y + 6, 4, rowHeight - 16, color);
      // The focus bar overlaps the permanent one-pixel outline.
      d.drawFastVLine(16, y + 6, rowHeight - 16, TFT_BLACK);
#endif
    } else {
#if TOUCH_CARD_PAPER_MONO
      d.fillRoundRect(16, y + 24, 4, rowHeight - 48, 2, color);
#else
      d.drawRoundRect(10, y, d.width() - 20, rowHeight - 2, 5, color);
#endif
    }
  }
  d.endWrite();
}
bool loadOwn() {
  tc::Bytes payload;
  shown.clear();
  return storage.own.load("own", payload) && tc::parse(payload, shown) &&
         tc::profileValid(shown["profile"]);
}
void landscapeCardView(JsonVariantConst p) {
  auto &d = M5.Display;
  const auto layout = tc::landscapeLayout(design);
  const bool contrast = design == 1;
  if (layout.band.w)
    d.fillRect(layout.band.x, layout.band.y, layout.band.w, layout.band.h,
               TFT_BLACK);
  avatarPhoto(shown["avatar"], layout.avatar.x, layout.avatar.y,
              layout.avatar.w, contrast ? TFT_BLACK : TFT_WHITE);
  wrap(p["name"].as<String>(), layout.name.x, layout.name.y, layout.name.w,
       layout.name.h / (20 * layout.nameSize), layout.nameSize,
       contrast ? TFT_WHITE : TFT_BLACK,
       contrast ? TFT_BLACK : TFT_WHITE);
  if (layout.accent.w)
    d.fillRect(layout.accent.x, layout.accent.y, layout.accent.w,
               layout.accent.h, TFT_BLACK);
  const auto field = [&](const char *key, const tc::CardRect &box) {
    wrap(p[key].as<String>(), box.x, box.y, box.w, box.h / 20);
  };
  field("account", layout.account);
  field("email", layout.email);
  field("comment", layout.comment);
  if (!p["url"].as<String>().isEmpty())
    d.qrcode(p["url"].as<const char *>(), layout.qr.x, layout.qr.y,
             layout.qr.w, 1, true);
  footer(screen == Screen::Card ? tc::tr("詳細・QR・削除", "Details") : tc::tr("名刺交換", "Card exchange"),
         tc::tr("メニュー", "Menu"));
}
bool cardQrTapped(int x, int y) {
  if (!paper() || shown["profile"]["url"].as<String>().isEmpty())
    return false;
  return tc::paperCardLayout(design, landscapeCard()).qr.contains(x, y);
}
void cardView() {
  auto &d = M5.Display;
  bool large = paper();
  int w = d.width(), h = d.height();
  if (!tc::profileValid(shown["profile"])) {
    wrap(tc::tr("スマホで自分の名刺を作り、設定からNFCで更新してください。", "Create your card on your phone, then open Phone update in Settings."), 24, h / 3,
         w - 48, 4, large ? 2 : 1);
    footer("", tc::tr("メニュー", "Menu"));
    return;
  }
  auto p = shown["profile"];
#if TOUCH_CARD_PAPER_MONO
  tc::PaperProfile profile{p["name"] | "", p["account"] | "", p["email"] | "",
                          p["comment"] | "", p["url"] | ""};
  tc::drawPaperCard(d, profile, design, landscapeCard(),
                    [&](tc::CardRect box, uint32_t background) {
    avatarPhoto(shown["avatar"], box.x, box.y, box.w, background);
  });
  footer(screen == Screen::Card ? tc::tr("詳細・QR・削除", "Details") : tc::tr("名刺交換", "Card exchange"),
         tc::tr("メニュー", "Menu"));
  return;
#endif
  if (landscapeCard()) {
    landscapeCardView(p);
    return;
  }
  String name = p["name"].as<String>(), account = p["account"].as<String>(),
         email = p["email"].as<String>(), comment = p["comment"].as<String>();
  if (!large) {
    bool contrast = design == 1;
    uint32_t fg = contrast ? TFT_WHITE : TFT_BLACK,
             bg = contrast ? TFT_BLACK : TFT_WHITE;
    if (contrast)
      d.fillRoundRect(12, 40, w - 24, 148, 6, TFT_BLACK);
    int iconX = design == 2 ? w - 80 : 24, nameX = design == 2 ? 24 : 104;
    avatarPhoto(shown["avatar"], iconX, 52, 56, bg);
    wrap(name, nameX, 52, design == 2 ? w - 120 : w - 128, 2, 1, fg, bg);
    wrap(account, nameX, 100, design == 2 ? w - 120 : w - 128, 1, 1, fg, bg);
    d.drawFastHLine(24, 127, w - 48, fg);
    wrap(email, 24, 140, w - 48, 1, 1, fg, bg);
    wrap(comment, 24, 166, w - 48, 1, 1, fg, bg);
    footer(screen == Screen::Card ? tc::tr("詳細・QR・削除", "Details") : tc::tr("名刺交換", "Card exchange"),
           tc::tr("メニュー", "Menu"));
    return;
  }
  int top = large ? 100 : 52, avatar = large ? 128 : 72;
  if (design == 1) {
    d.fillRect(16, 44, w - 32, large ? 268 : 114, TFT_BLACK);
    avatarPhoto(shown["avatar"], w - avatar - 32, top, avatar, TFT_BLACK);
    wrap(name, 32, top, w - avatar - 80, large ? 3 : 2, large ? 2 : 1,
         TFT_WHITE, TFT_BLACK);
    wrap(account, 32, large ? 256 : 128, w - 64, 1, 1, TFT_WHITE, TFT_BLACK);
    wrap(email, 32, large ? 348 : 168, w - 64, 1);
    wrap(comment, 32, large ? 394 : 188, w - 64, large ? 3 : 1);
  } else if (design == 2) {
    avatarPhoto(shown["avatar"], (w - avatar) / 2, top, avatar);
    int y = top + avatar + 20;
    wrap(name, 24, y, w - 48, large ? 2 : 1, large ? 2 : 1);
    y += large ? 90 : 24;
    wrap(account, 24, y, w - 48, 1);
    if (large) {
      wrap(email, 24, y + 36, w - 48, 2);
      wrap(comment, 24, y + 96, w - 48, 2);
    }
  } else {
    avatarPhoto(shown["avatar"], w - avatar - 24, top, avatar);
    wrap(name, 24, top, w - avatar - 72, large ? 3 : 2, large ? 3 : 1);
    int y = large ? 290 : 140;
    d.fillRect(24, y, large ? 80 : 40, 4, TFT_BLACK);
    wrap(account, 24, y + 22, w - 48, 1);
    wrap(email, 24, y + 52, w - 48, large ? 2 : 1);
    if (large)
      wrap(comment, 24, y + 110, w - 48, 3);
  }
  if (large && !p["url"].as<String>().isEmpty()) {
    d.qrcode(p["url"].as<const char *>(), 24, h - 230, 168, 1, true);
    text(tc::tr("タップでQRを拡大", "Tap to enlarge QR"), 212, h - 150);
  }
  footer(screen == Screen::Card ? tc::tr("詳細・QR・削除", "Details") : tc::tr("名刺交換", "Card exchange"),
         tc::tr("メニュー", "Menu"));
}
void dashboardBatteryStatus() {
  auto &d = M5.Display;
  const auto *font = d.getFont();
#if TOUCH_CARD_PAPER_MONO
  dashboardBattery = M5.Power.getBatteryLevel();
  tc::drawPaperBattery(d, dashboardBattery);
#else
  d.setFont(font24());
  d.setTextSize(1);
  d.fillRect(d.width() - 90, 0, 90, 30, TFT_WHITE);
  dashboardBattery = M5.Power.getBatteryLevel();
  String value = dashboardBattery < 0 ? "--%" : String(dashboardBattery) + "%";
  text(value, d.width() - 16 - d.textWidth(value), 6);
#endif
  d.setFont(font);
}
void clockDashboard(bool includeBattery = true) {
  auto now = clockValue.localTime();
  auto &d = M5.Display;
#if TOUCH_CARD_PAPER_MONO
  tc::drawPaperHomeTime(d, now.value, now.valid);
  if (includeBattery) dashboardBatteryStatus();
  return;
#endif
  int x = paper() ? 24 : 144, y = paper() ? 488 : 40,
      w = paper() ? d.width() - 48 : 160;
  d.fillRect(x, y, w, paper() ? 150 : 110, TFT_WHITE);
  char time[16] = "--:--", date[40];
  snprintf(date, sizeof(date), "%s", tc::tr("日時未設定", "Time not set"));
  if (now.valid) {
    strftime(time, sizeof(time), "%H:%M", &now.value);
    snprintf(date, sizeof(date), "%d/%d (%s)", now.value.tm_mon + 1,
             now.value.tm_mday,
             std::vector<const char *>{tc::tr("日", "Su"), tc::tr("月", "Mo"), tc::tr("火", "Tu"), tc::tr("水", "We"), tc::tr("木", "Th"), tc::tr("金", "Fr"),
                                       tc::tr("土", "Sa")}[now.value.tm_wday]);
  }
#if TOUCH_CARD_PAPER_MONO
  d.setFont(&tc::paper96);
  text(time, x, y);
#else
  d.setFont(&tc::stackClockDigits);
  text(time, x, y);
#endif
  d.setFont(font16());
  text(date, x, y + (paper() ? 100 : 62), paper() ? 2 : 1);
  if (includeBattery)
    dashboardBatteryStatus();
}
#if TOUCH_CARD_PAPER_MONO
void refreshPaperDate(const tm &date, bool valid) {
  M5.Display.setEpdMode(epd_mode_t::epd_fastest);
  tc::updatePaperDate(M5.Display, date, valid, screen, home);
}
#endif
void week() {
  auto now = clockValue.localTime();
  if (!now.valid)
    return;
  auto &d = M5.Display;
  d.setFont(paper() ? font24() : font16());
  int y = paper() ? 660 : 154, cell = (d.width() - 32) / 7;
  int today = now.value.tm_mday;
  for (int i = 0; i < 7; i++) {
    tm date = now.value;
    date.tm_mday += i - now.value.tm_wday;
    mktime(&date);
    int x = 16 + i * cell;
    if (date.tm_mday == today)
      d.drawRoundRect(x, y, cell - 4, 36, 3, TFT_BLACK);
    text(String(date.tm_mday), x + 10, y + 10);
  }
}
void dashboard() {
  tc::Bytes b;
  JsonDocument doc;
  if (storage.own.load("dashboard", b))
    tc::parse(b, doc);
#if TOUCH_CARD_PAPER_MONO
  const auto box = tc::paperHomePhoto;
  photo(doc["image"], box.x, box.y, box.w, box.h);
  const auto now = clockValue.localTime();
  tc::drawPaperHomeDetails(M5.Display, now.value, now.valid);
  clockDashboard();
#else
  photo(doc["image"], paper() ? 24 : 16, paper() ? 32 : 36, paper() ? 432 : 112,
        paper() ? 432 : 112);
  clockDashboard();
  week();
#endif
  footer(tc::tr("カレンダー", "Calendar"), tc::tr("メニュー", "Menu"));
}
void month() {
  auto now = clockValue.localTime();
#if TOUCH_CARD_PAPER_MONO
  tc::drawPaperMonth(M5.Display, now.value, now.valid);
  footer(tc::tr("ホーム", "Home"), "");
  return;
#endif
  if (!now.valid) {
    text(tc::tr("先に日時を合わせてください", "Set the date and time first"), 20, 90);
    return;
  }
  auto &d = M5.Display;
  tm start = now.value;
  start.tm_mday = 1;
  mktime(&start);
  char title[32];
  snprintf(title, sizeof(title), tc::tr("%d年 %d月", "%d / %02d"), start.tm_year + 1900,
           start.tm_mon + 1);
  text(title, 24, 42, paper() ? 2 : 1);
  const char *names[] = {tc::tr("日", "Su"), tc::tr("月", "Mo"), tc::tr("火", "Tu"), tc::tr("水", "We"), tc::tr("木", "Th"), tc::tr("金", "Fr"), tc::tr("土", "Sa")};
  int cw = (d.width() - 32) / 7, ch = paper() ? 75 : 21,
      top = paper() ? 150 : 84;
  for (int i = 0; i < 7; i++)
    text(names[i], 16 + i * cw, top - 24);
  for (int date = 1; date <= 31; date++) {
    tm day = start;
    day.tm_mday = date;
    mktime(&day);
    if (day.tm_mon != start.tm_mon)
      break;
    int slot = start.tm_wday + date - 1, x = 16 + (slot % 7) * cw,
        y = top + (slot / 7) * ch;
    if (date == now.value.tm_mday)
      d.drawRect(x - 2, y - 2, cw - 4, ch - 2, TFT_BLACK);
    text(String(date), x + 4, y, paper() ? 2 : 1);
  }
  footer(tc::tr("ホーム", "Home"), "");
}
void render();
void navigate(Screen next) {
  if (next != Screen::Nfc) mutual.reset();
  screen = next;
  focus = 0;
  render();
}
void notice(const String &s) {
  message = s;
  navigate(Screen::Message);
}
void showReceivedCard(const String &id) {
  tc::Bytes data;
  shown.clear();
  selectedId = "";
  if (id.length() && storage.loadCard(id.c_str(), data) &&
      tc::parse(data, shown) && tc::profileValid(shown["profile"])) {
    selectedId = id;
    navigate(Screen::Card);
  } else {
    // For example, the SD card may have been removed after the durable save.
    // Do not display a stale card or imply that the completed exchange failed.
    notice(tc::tr("受け取った名刺を表示できません。保存先を確認して名刺帳から開いてください", "Cannot display the received card. Check storage and open it from Cards."));
  }
}
void startBook(const tc::Entry *cursor = nullptr, bool reverse = false) {
  storage.catalog.pageSize = paper() ? 5 : 3;
  storage.catalog.start(nameSort, cursor, reverse);
  navigate(Screen::Book);
}
void render() {
  auto &d = M5.Display;
  if (paper()) {
    const uint8_t rotation = landscapeCard() ? 1 : 0;
    if (d.getRotation() != rotation) {
      d.waitDisplay();
      d.setRotation(rotation);
    }
  }
  d.setFont(paper() && !tc::hasGrayContent(screen) ? font24() : font16());
  d.setTextSize(1);
#if TOUCH_CARD_PAPER_MONO
  const auto refresh = tc::paperRefresh(displayInitialized, screen);
  tc::setPaperFontMonochrome(refresh == tc::PaperRefresh::Differential);
  d.setEpdMode(refresh == tc::PaperRefresh::Clean ? epd_mode_t::epd_quality
               : refresh == tc::PaperRefresh::Gray ? epd_mode_t::epd_fast
                                                   : epd_mode_t::epd_fastest);
#endif
  d.startWrite();
  d.fillScreen(TFT_WHITE);
  labels.clear();
  if (screen != Screen::Home || home == 0)
    status();
#if TOUCH_CARD_PAPER_MONO
  // Status glyphs use monochrome partial updates; retain smooth grayscale
  // typography for the card body in a normal card render.
  tc::setPaperFontMonochrome(refresh == tc::PaperRefresh::Differential);
#endif
  switch (screen) {
  case Screen::Home:
    if (home == 0) {
      loadOwn();
      cardView();
    } else if (home == 1)
      dashboard();
    else {
      tc::Bytes b;
      JsonDocument image;
      if (storage.own.load("fullscreen", b))
        tc::parse(b, image);
      photo(image["image"], 0, 0, d.width(), d.height());
    }
    break;
  case Screen::Preview:
    loadOwn();
    cardView();
    break;
  case Screen::Card:
    cardView();
    break;
  case Screen::Menu:
    heading(tc::tr("メニュー", "Menu"));
    rows({tc::tr("名刺交換", "Card exchange"), tc::tr("名刺帳", "Cards"), tc::tr("ホーム表示", "Home screen"), tc::tr("設定", "Settings")});
    footer(tc::tr("ホームへ戻る", "Home"), "");
    break;
  case Screen::Exchange:
    heading(tc::tr("名刺交換", "Card exchange"));
    rows({tc::tr("交換", "Exchange"), tc::tr("渡す", "Send"), tc::tr("受け取る", "Receive"), tc::tr("戻る", "Back")});
    break;
  case Screen::MutualChoice:
    heading(tc::tr("交換", "Exchange"));
    rows({tc::tr("先に渡す", "Send first"), tc::tr("先に受け取る", "Receive first"), tc::tr("戻る", "Back")});
    footer(tc::tr("相手と逆の順番を選択", "Choose opposite roles"), "");
    break;
  case Screen::HomeChoice:
    heading(tc::tr("ホーム表示", "Home screen"));
    rows({tc::tr("名刺", "Card"), tc::tr("画像＋日時・カレンダー", "Photo + clock"), tc::tr("全画面写真", "Full-screen photo"), tc::tr("戻る", "Back")});
    break;
  case Screen::Settings:
    heading(tc::tr("設定", "Settings"));
    if (paper())
      rows({tc::tr("名刺デザイン", "Card design"), landscape ? tc::tr("名刺の向き：横", "Orientation: landscape") : tc::tr("名刺の向き：縦", "Orientation: portrait"),
            tc::tr("ストレージ", "Storage"), tc::tr("スマホから更新", "Phone update"), tc::tr("その他", "More")});
    else
      rows({tc::tr("名刺デザイン", "Card design"), tc::tr("ストレージ", "Storage"), tc::tr("スマホから更新", "Phone update"), tc::tr("その他", "More")});
    footer(tc::tr("戻る", "Back"), "");
    break;
  case Screen::SystemSettings:
    heading(tc::tr("設定", "Settings"));
    rows({"言語 / Language", tc::tr("電源", "Power"), tc::tr("戻る", "Back")});
    break;
  case Screen::Language:
    heading("言語 / Language");
    rows({String(tc::uiLanguage == tc::Language::Japanese ? "● " : "  ") + "日本語",
          String(tc::uiLanguage == tc::Language::English ? "● " : "  ") + "English", tc::tr("戻る", "Back")});
    break;
  case Screen::Power:
    heading(tc::tr("電源", "Power"));
    rows({tc::tr("電源オフ", "Power off"), tc::tr("再起動", "Restart"), tc::tr("戻る", "Back")});
    break;
  case Screen::PowerOff:
  case Screen::Restart:
    heading(screen == Screen::Restart ? tc::tr("再起動", "Restart") : tc::tr("電源オフ", "Power off"));
    wrap(screen == Screen::Restart
         ? tc::tr("再起動しますか？名刺や設定は消去しません。", "Restart? Saved cards and settings will be kept.")
         : tc::tr("電源を切りますか？名刺や設定は消去しません。", "Power off? Saved cards and settings will be kept."),
         24, 90, d.width() - 48, paper() ? 6 : 4);
    footer(tc::tr("キャンセル", "Cancel"), tc::tr("実行", "Confirm"));
    break;
  case Screen::Orientation:
    heading(tc::tr("名刺の向き", "Orientation"));
    rows({String(!landscape ? "● " : "  ") + tc::tr("縦向き", "Portrait"),
          String(landscape ? "● " : "  ") + tc::tr("横向き", "Landscape"), tc::tr("戻る", "Back")});
    break;
  case Screen::Design:
    heading(tc::tr("名刺デザイン", "Card design"));
    rows({String(design == 0 ? "● " : "  ") + tc::tr("タイポグラフィ", "Typography"),
          String(design == 1 ? "● " : "  ") + tc::tr("コントラスト", "Contrast"),
          String(design == 2 ? "● " : "  ") + tc::tr("シンプル", "Simple"), tc::tr("戻る", "Back")});
    break;
  case Screen::Storage:
    heading(tc::tr("ストレージ", "Storage"));
    rows({tc::tr("本体", "Internal"), "microSD",
          storage.internalOnly ? tc::tr("保存先：本体", "Storage: internal") : tc::tr("保存先：自動（SD優先）", "Storage: auto (SD first)"),
          tc::tr("戻る", "Back")});
    break;
  case Screen::Internal:
  case Screen::Sd: {
    bool sd = screen == Screen::Sd;
    auto &io = sd ? storage.sd : storage.internal;
    auto &u = sd ? storage.board.sd : storage.board.internal;
    heading(sd ? "microSD" : tc::tr("本体ストレージ", "Internal storage"));
    if (io.ready) {
      uint64_t free = io.freeBytes();
      uint64_t total = u.total;
      text(tc::tr("使用 ", "Used ") + bytes(total > free ? total - free : 0) + " / " +
               bytes(total),
           20, 78);
      text(tc::tr("空き ", "Free ") + bytes(free), 20, 106);
      uint64_t reserved = sd ? 65536 : BoardStorage::kInternalReserve;
      text(tc::tr("名刺用 ", "For cards ") + bytes(free > reserved ? free - reserved : 0), 20, 134);
      text(io.writable ? tc::tr("利用可能", "Available") : tc::tr("読み取り専用", "Read only"), 20, 162);
      footer(tc::tr("内訳", "Usage"), sd ? tc::tr("取り外す", "Eject") : tc::tr("戻る", "Back"));
    } else {
      wrap(
          sd ? (storage.sdEjected ? tc::tr("取り外し可能", "Safe to remove") : tc::tr("SD未装着・認識できません", "SD absent or unavailable"))
             : tc::tr("本体データ領域を認識できません。初期化すると本体データを消去します。", "Internal storage is unavailable. Formatting erases internal data."),
          20, 80, d.width() - 40, 5);
      footer(sd ? tc::tr("再読み込み", "Reload") : tc::tr("初期化の確認", "Format…"), tc::tr("戻る", "Back"));
    }
    break;
  }
  case Screen::Usage: {
    auto &c = storage.catalog;
    heading(usageMedium ? tc::tr("microSD 内訳", "microSD usage") : tc::tr("本体 内訳", "Internal usage"), 20);
    if (c.scanning)
      text(tc::tr("集計中…", "Counting…"), 20, 90);
    else {
      text(tc::tr("名刺 ", "Cards: ") + String((unsigned long)c.counts[usageMedium]) + tc::tr("件", ""), 20,
           78);
      text(tc::tr("名刺（現行） ", "Cards (current) ") + bytes(c.bytes[usageMedium]), 20, 106);
      text(tc::tr("アプリ全体 ", "App total ") + bytes(c.appBytes[usageMedium]), 20, 134);
      text(tc::tr("旧世代・索引も含みます", "Includes versions and indexes"), 20, 162);
    }
    footer(tc::tr("戻る", "Back"), "");
    break;
  }
  case Screen::Book: {
    heading(nameSort ? tc::tr("名刺帳・名前順", "Cards: name") : tc::tr("名刺帳・最新順", "Cards: newest"), 20);
    const auto back = tc::bookBackRect(d.width(), paper());
    d.drawRoundRect(back.x, back.y, back.w, back.h, 5, TFT_BLACK);
    const auto *font = d.getFont();
    if (paper())
      d.setFont(font24());
    d.setTextSize(1);
    text(tc::tr("戻る", "Back"), back.x + (back.w - d.textWidth(tc::tr("戻る", "Back"))) / 2,
         back.y + (paper() ? 10 : 8));
    d.setFont(font);
    if (storage.catalog.scanning)
      text(tc::tr("読み込み中…", "Loading…"), 20, 100);
    else {
      rowTop = paper() ? 100 : 72;
      rowHeight = paper() ? 120 : 40;
      for (size_t i = 0; i < storage.catalog.page.size(); i++) {
        auto &e = storage.catalog.page[i];
        int y = rowTop + i * rowHeight;
#if !TOUCH_CARD_PAPER_MONO
        d.drawRoundRect(16, y, d.width() - 32, rowHeight - 4, 4, TFT_BLACK);
        if (int(i) == focus)
          d.fillRect(16, y + 6, 4, rowHeight - 16, TFT_BLACK);
#endif
        tc::Bytes payload;
        JsonDocument preview;
        if (storage.loadCard(e.key, payload))
          tc::parse(payload, preview);
#if TOUCH_CARD_PAPER_MONO
        tc::drawPaperBookEntry(d, e.name.c_str(), e.account.c_str(), y, int(i) == focus,
                              [&](tc::CardRect box) {
          avatarPhoto(preview["avatar"], box.x, box.y, box.w);
        });
#else
        int avatar = paper() ? 88 : 28;
        avatarPhoto(preview["avatar"], paper() ? 32 : 26, y + (paper() ? 16 : 6), avatar);
        int labelX = paper() ? 144 : 66;
        d.setFont(font16());
        wrap(e.name.c_str(), labelX, y + 8, d.width() - labelX - 24, 1,
             paper() ? 2 : 1);
        if (paper()) {
          d.setFont(font24());
          wrap(e.account.c_str(), labelX, y + 58, d.width() - labelX - 24, 1);
          d.setFont(font16());
        }
#endif
        labels.push_back(e.name.c_str());
      }
      if (storage.catalog.page.empty())
        text(tc::tr("名刺はありません", "No cards yet"), 20, 100);
    }
    footer(tc::tr("並び順", "Sort"), tc::tr("次へ", "Next"));
    d.setFont(font24());
    d.setTextSize(1);
    text(tc::tr("前へ", "Previous"), (d.width() - d.textWidth(tc::tr("前へ", "Previous"))) / 2, d.height() - 34);
    break;
  }
  case Screen::CardActions:
    heading(tc::tr("名刺の操作", "Card actions"), 20);
    if (selectedId.isEmpty())
      rows({tc::tr("全文を読む", "Read full text"), tc::tr("QRを表示", "Show QR"), tc::tr("戻る", "Back")});
    else
      rows({tc::tr("全文を読む", "Read full text"), tc::tr("QRを表示", "Show QR"), tc::tr("この名刺を削除", "Delete card"), tc::tr("戻る", "Back")});
    break;
  case Screen::Delete: {
    heading(tc::tr("名刺を削除", "Delete card"), 20);
    wrap(shown["profile"]["name"].as<String>(), 20, 80, d.width() - 40, 2);
    tc::Entry e;
    String where = tc::tr("対象：", "Storage: ");
    deleteMask = 0;
    deleteInternalGeneration = storage.internal.generation;
    deleteSdGeneration = storage.sd.generation;
    if (storage.own.latest(selectedId.c_str(), e)) {
      where += tc::tr("本体 ", "Internal ");
      deleteMask |= 1;
    }
    if (storage.sd.ready && storage.external.latest(selectedId.c_str(), e)) {
      where += "SD";
      deleteMask |= 2;
    }
    text(where, 20, 132);
    wrap(tc::tr("元に戻せません。未接続のSDは対象外です。", "Cannot be undone. Disconnected SD cards are not affected."), 20, paper() ? 190 : 156,
         d.width() - 40, 2);
    footer(tc::tr("キャンセル", "Cancel"), tc::tr("削除する", "Delete"));
    break;
  }
  case Screen::Text: {
    String all;
    for (const char *key : {"name", "account", "email", "comment", "url"}) {
      String value = shown["profile"][key].as<String>();
      if (!value.isEmpty())
        all += value + "\n\n";
    }
    auto parts = lines(all, d.width() - 40);
    const int step = paper() ? 30 : 22;
    int count = (d.height() - 110) / step;
    int begin = textPage * count;
    if (begin >= int(parts.size())) {
      textPage = 0;
      begin = 0;
    }
    for (int i = 0; i < count && begin + i < int(parts.size()); i++)
      text(parts[begin + i], 20, 50 + i * step);
    footer(tc::tr("次のページ", "Next page"), tc::tr("戻る", "Back"));
    break;
  }
  case Screen::Qr: {
    String url = shown["profile"]["url"].as<String>();
    if (url.isEmpty())
      text(tc::tr("QR用URLが未設定です", "No QR URL set"), 20, 90);
    else {
      int size = std::min(d.width() - 32, d.height() - 78);
#if TOUCH_CARD_PAPER_MONO
      tc::drawPaperQr(d, url.c_str(), {(d.width() - size) / 2, 34, size, size});
#else
      d.qrcode(url.c_str(), (d.width() - size) / 2, 34, size, 1, true);
#endif
    }
    footer(tc::tr("戻る", "Back"), "");
    break;
  }
  case Screen::Month:
    month();
    break;
  case Screen::Nfc:
    heading(mutual.active ? String(tc::tr("交換 ", "Exchange ")) + String(mutual.leg) + "/2・" + (nfcUiSender ? tc::tr("渡す", "Send") : tc::tr("受け取る", "Receive"))
                          : nfcUiSender ? tc::tr("名刺を渡す", "Send card") : nfcUiPhone ? tc::tr("スマホから更新", "Phone update") : tc::tr("名刺を受け取る", "Receive card"), 20);
    if (paper())
      wrap(message, 32, 128, d.width() - 64, 4, 1);
    else {
      const char *hint = mutual.active ? (mutual.switching ? tc::tr("役割を切り替え中・そのままタッチ", "Switching roles. Keep touching.") : tc::tr("2枚目の完了まで離さないでください", "Keep touching for both cards.")) : !nfcUiSender && !nfcUiPhone
          ? (storage.internalOnly || !storage.sd.ready || !storage.sd.writable ? tc::tr("保存先：本体", "Storage: internal") : tc::tr("保存先：SD優先", "Storage: SD first"))
          : tc::tr("完了まで離さないでください", "Keep touching until done.");
      text(hint, 24, 62);
    }
    transferProgress(true);
    footer(tc::tr("キャンセル", "Cancel"), "");
    break;
  case Screen::Format:
    wrap(tc::tr("本体データをすべて消去して初期化します。名刺や画像は復元できません。SDは消去しません。", "Erase all internal cards and images? This cannot be undone. SD data will be kept."),
         24, 64, d.width() - 48, paper() ? 8 : 5);
    footer(tc::tr("キャンセル", "Cancel"), tc::tr("消去して初期化", "Erase & format"));
    break;
  case Screen::Message:
    wrap(message, 24, 76, d.width() - 48, paper() ? 12 : 5, paper() ? 2 : 1);
    footer(tc::tr("ホーム", "Home"), "");
    break;
  }
#if TOUCH_CARD_PAPER_MONO
  // Keep controls hidden after background NFC completion or a date change,
  // including book pagination drawn outside footer(). Do not touch photos.
  paperFooterCache.capture(d, screen, home);
  tc::clearSleepingPaperFooter(d, screenBlanker.off, screen, home);
#endif
  d.endWrite();
  displayInitialized = true;
  auto now = clockValue.localTime();
  lastMinute = now.valid ? now.value.tm_yday * 1440 + now.value.tm_hour * 60 +
                              now.value.tm_min
                        : int(millis() / 60000) + 600000;
  lastDay = now.valid ? now.value.tm_yday : -1;
  lastStatusUpdate = millis();
}
void waitNfc(bool phone, bool sender = false, bool mutualMode = false) {
  if (!mutualMode) mutual.reset();
  // Preserve the first leg's received ID while sending the second leg, but
  // never carry a previous exchange's card into a new session.
  if (!mutualMode || mutual.leg == 1) cards.lastReceivedId = "";
  nfc.stop();
  nfcUiSender = sender;
  nfcUiPhone = phone;
  nfcProgressRefresh.reset();
  message = sender  ? tc::tr("相手を『受け取る』にしてタッチしてください", "Select Receive on the other device, then touch.")
            : phone ? tc::tr("スマホから名刺・画像・日時を更新します", "Update cards, photos or time from your phone.")
                    : tc::tr("相手を『渡す』にしてタッチしてください", "Select Send on the other device, then touch.");
  if (mutualMode) message = mutual.leg == 1
      ? tc::tr("相手も「交換」を開き、逆の順番を選んでタッチしてください。2枚目まで自動で交換します", "Open Exchange on both devices, choose opposite roles and touch. Both cards will be sent.")
      : tc::tr("1枚目は完了しました。続けて2枚目を交換します。そのままタッチしてください", "First card complete. Keep touching for the second card.");
  if (!phone && !sender)
    message += storage.internalOnly || !storage.sd.ready || !storage.sd.writable
                   ? tc::tr("\n保存先：本体", "\nStorage: internal")
                   : tc::tr("\n保存先：SD優先（容量不足なら本体）", "\nStorage: SD first (internal if full)");
  navigate(Screen::Nfc);
  const uint8_t leg = mutualMode ? mutual.leg : 0;
  if (!(sender ? nfc.sendOwn(leg, mutual.token) : nfc.open(phone, leg, mutual.token))) {
    const String error = (mutualMode && leg == 2 ? tc::tr("1枚目は完了しています。\n", "First card completed.\n") : "") + nfc.error;
    nfc.stop();
    notice(error);
  }
}
void cancelNfc() {
  const bool paired = mutual.active;
  const bool partial = mutual.sent || mutual.received || nfc.done;
  nfc.stop();
  if (paired) notice(partial ? tc::tr("交換を中止しました。完了した分の名刺は保存先に残っています", "Exchange cancelled. Completed cards remain saved.")
                            : tc::tr("交換を中止しました", "Exchange cancelled"));
  else navigate(Screen::Home);
}
void activate(int row) {
  switch (screen) {
  case Screen::Menu:
    if (row == 0)
      navigate(Screen::Exchange);
    else if (row == 1)
      startBook();
    else if (row == 2)
      navigate(Screen::HomeChoice);
    else
      navigate(Screen::Settings);
    break;
  case Screen::Exchange:
    if (row == 0)
      navigate(Screen::MutualChoice);
    else if (row == 1)
      waitNfc(false, true);
    else if (row == 2)
      waitNfc(false);
    else
      navigate(Screen::Menu);
    break;
  case Screen::MutualChoice:
    if (row == 2) navigate(Screen::Exchange);
    else {
      nfc.stop();
      if (!nfc.prepareOwn()) { notice(nfc.error); break; }
      mutual.start(row == 0);
      waitNfc(false, mutual.sender(), true);
    }
    break;
  case Screen::Settings: {
    if (paper() && row == 1) {
      navigate(Screen::Orientation);
      break;
    }
    if (paper() && row > 1)
      --row;
    if (row == 0)
      navigate(Screen::Design);
    else if (row == 1)
      navigate(Screen::Storage);
    else if (row == 2)
      waitNfc(true);
    else
      navigate(Screen::SystemSettings);
    break;
  }
  case Screen::SystemSettings:
    navigate(row == 0 ? Screen::Language : row == 1 ? Screen::Power : Screen::Settings);
    break;
  case Screen::Language:
    if (row == 2) navigate(Screen::SystemSettings);
    else if (row >= 0 && row <= 1) {
      if (!prefs || settings.putUChar("language", row) != 1) notice(tc::tr("設定を保存できません", "Cannot save settings"));
      else {
        tc::uiLanguage = tc::resolveLanguage(row);
        Serial.printf("[tc.ui.language] saved=%s\n", row == 1 ? "en" : "ja");
        navigate(Screen::SystemSettings);
      }
    }
    break;
  case Screen::Power:
    navigate(row == 0 ? Screen::PowerOff : row == 1 ? Screen::Restart : Screen::SystemSettings);
    break;
  case Screen::Orientation:
    if (!paper() || row == 2)
      navigate(Screen::Settings);
    else if (row < 0 || row > 1)
      break;
    else if (!prefs || settings.putBool("landscape", row == 1) != 1)
      notice(tc::tr("設定を保存できません", "Cannot save settings"));
    else {
      landscape = row == 1;
      navigate(Screen::Preview);
    }
    break;
  case Screen::HomeChoice:
    if (row == 3)
      navigate(Screen::Menu);
    else if (!prefs || settings.putUChar("home", row) != 1)
      notice(tc::tr("設定を保存できません", "Cannot save settings"));
    else {
      home = row;
      navigate(Screen::Home);
    }
    break;
  case Screen::Design:
    if (row == 3)
      navigate(Screen::Settings);
    else if (!prefs || settings.putUChar("design", row) != 1)
      notice(tc::tr("設定を保存できません", "Cannot save settings"));
    else {
      design = row;
      navigate(Screen::Preview);
    }
    break;
  case Screen::Storage:
    if (row == 0) {
      storage.board.refreshInternal();
      navigate(Screen::Internal);
    } else if (row == 1)
      navigate(Screen::Sd);
    else if (row == 2) {
      if (!prefs ||
          settings.putBool("internalOnly", !storage.internalOnly) != 1)
        notice(tc::tr("設定を保存できません", "Cannot save settings"));
      else {
        storage.internalOnly = !storage.internalOnly;
        render();
      }
    } else
      navigate(Screen::Settings);
    break;
  case Screen::Book:
    if (!storage.catalog.scanning && row < int(storage.catalog.page.size())) {
      selectedId = storage.catalog.page[row].key.c_str();
      tc::Bytes data;
      shown.clear();
      if (!storage.loadCard(selectedId.c_str(), data) ||
          !tc::parse(data, shown))
        notice(tc::tr("この名刺を読み込めません。SDを確認してください", "Cannot read this card. Check the SD card."));
      else
        navigate(Screen::Card);
    }
    break;
  case Screen::CardActions:
    if (row == 0) {
      textPage = 0;
      detailReturn = selectedId.isEmpty() ? Screen::Home : Screen::Card;
      navigate(Screen::Text);
    } else if (row == 1) {
      detailReturn = selectedId.isEmpty() ? Screen::Home : Screen::Card;
      navigate(Screen::Qr);
    } else if (row == 2 && !selectedId.isEmpty())
      navigate(Screen::Delete);
    else
      navigate(selectedId.isEmpty() ? Screen::Home : Screen::Card);
    break;
  default:
    break;
  }
}
void tap(int x, int y) {
  int w = M5.Display.width(), h = M5.Display.height();
  bool left = x < w / 2, bottom = y >= h - 44;
  if (screen == Screen::Settings && bottom) { navigate(Screen::Menu); return; }
  if (screen == Screen::PowerOff || screen == Screen::Restart) {
    if (!bottom) return;
    if (left) { navigate(Screen::Power); return; }
    const bool restart = screen == Screen::Restart;
    nfc.stop();
    M5.Display.waitDisplay();
    if (restart) ESP.restart();
    else M5.Power.powerOff();
    return;
  }
  if (screen == Screen::Menu && tc::menuHomeRect(w, h).contains(x, y)) {
    navigate(Screen::Home);
    return;
  }
  if (screen == Screen::Nfc) {
    if (bottom) {
      cancelNfc();
    }
    return;
  }
  if (screen == Screen::Home) {
    if (home == 2) {
      navigate(Screen::Menu);
      return;
    }
    if (home == 1) {
      navigate(bottom && !left ? Screen::Menu : Screen::Month);
      return;
    }
    if (!tc::profileValid(shown["profile"])) {
      navigate(Screen::Menu);
      return;
    }
    if (bottom) {
      navigate(left ? Screen::Exchange : Screen::Menu);
    } else if (cardQrTapped(x, y)) {
      detailReturn = Screen::Home;
      navigate(Screen::Qr);
    } else {
      selectedId = "";
      navigate(Screen::CardActions);
    }
    return;
  }
  if (screen == Screen::Preview) {
    navigate(bottom && !left ? Screen::Menu : Screen::Home);
    return;
  }
  if (screen == Screen::Card) {
    if (bottom && !left)
      navigate(Screen::Menu);
    else if (!bottom && cardQrTapped(x, y)) {
      detailReturn = Screen::Card;
      navigate(Screen::Qr);
    } else
      navigate(Screen::CardActions);
    return;
  }
  if (screen == Screen::Text) {
    if (bottom && !left)
      navigate(detailReturn);
    else {
      textPage++;
      render();
    }
    return;
  }
  if (screen == Screen::Qr) {
    navigate(detailReturn);
    return;
  }
  if (screen == Screen::Month || screen == Screen::Message) {
    navigate(Screen::Home);
    return;
  }
  if (screen == Screen::Delete) {
    if (bottom) {
      if (left)
        navigate(Screen::Card);
      else {
        bool ok = storage.erase(selectedId.c_str(), deleteMask,
                                deleteInternalGeneration, deleteSdGeneration);
        shown.clear();
        if (ok)
          startBook();
        else
          notice(tc::tr("削除が未完了です：", "Deletion incomplete: ") + storage.deleteFailure +
                 tc::tr("。保存先を確認してください", ". Check storage."));
      }
    }
    return;
  }
  if (screen == Screen::Format) {
    if (bottom) {
      if (left)
        navigate(Screen::Storage);
      else {
        LittleFS.end();
        if (LittleFS.format() && LittleFS.begin(false)) {
          storage.board.internalMounted = true;
          storage.bindInternal();
          storage.catalog.start(nameSort);
          notice(tc::tr("本体データ領域を初期化しました", "Internal storage formatted"));
        } else
          notice(tc::tr("初期化に失敗しました", "Formatting failed"));
      }
    }
    return;
  }
  if (screen == Screen::Internal || screen == Screen::Sd) {
    bool sd = screen == Screen::Sd;
    auto &io = sd ? storage.sd : storage.internal;
    if (bottom && left) {
      if (io.ready) {
        usageMedium = sd;
        storage.catalog.start(nameSort);
        navigate(Screen::Usage);
      } else if (sd) {
        storage.mount();
        render();
      } else
        navigate(Screen::Format);
    } else if (bottom && sd && io.ready) {
      storage.eject();
      render();
    } else
      navigate(Screen::Storage);
    return;
  }
  if (screen == Screen::Usage) {
    navigate(usageMedium ? Screen::Sd : Screen::Internal);
    return;
  }
  if (screen == Screen::Book && tc::bookBackRect(w, paper()).contains(x, y)) {
    navigate(Screen::Menu);
    return;
  }
  if (screen == Screen::Book && bottom) {
    if (storage.catalog.scanning)
      return;
    if (x < w / 3) {
      nameSort = !nameSort;
      if (prefs)
        settings.putBool("nameSort", nameSort);
      startBook();
    } else if (x < w * 2 / 3) {
      if (storage.catalog.page.empty())
        startBook();
      else {
        auto cursor = storage.catalog.page.front();
        startBook(&cursor, true);
      }
    } else if (!storage.catalog.page.empty()) {
      auto cursor = storage.catalog.page.back();
      startBook(&cursor);
    } else
      startBook();
    return;
  }
  if (y < 34 && screen != Screen::Book) {
    navigate(Screen::Home);
    return;
  }
  const int row = tc::menuRowAt(x, y, w, rowTop, rowHeight, labels.size());
  if (row >= 0)
    activate(row);
}
} // namespace
void setup() {
#if TOUCH_CARD_PAPER_MONO
  const uint32_t bootStarted = millis();
#endif
  Serial.begin(115200);
  auto config = M5.config();
  config.internal_imu = false;
  config.internal_mic = false;
  config.internal_spk = false;
  config.internal_rtc = true;
  config.pmic_button = true;
#if TOUCH_CARD_PAPER_MONO
  // M5Unified otherwise clears twice: Display.init() and _begin(). Keep the
  // hardware initialization, but let our first render perform the only clean
  // refresh, with the final home/card pixels already prepared.
  config.clear_display = false;
#endif
  M5.begin(config);
  bool powerButtonProtected = true;
#if TOUCH_CARD_PAPER_MONO
  Serial.printf("[tc.boot] display_init_ms=%lu\n", (unsigned long)(millis() - bootStarted));
  const bool redLedOff = tc::turnOffPaperRedLed(
      [](uint8_t reg, uint8_t &value) { return M5.In_I2C.readRegister(0x6e, reg, &value, 1, 100000); },
      [](uint8_t reg, uint8_t value) { return M5.In_I2C.writeRegister(0x6e, reg, &value, 1, 100000); });
  Serial.printf("[tc.power] red_led_off=%u\n", redLedOff);
  powerButtonProtected = tc::protectPaperPowerButton(
      [](uint8_t reg, uint8_t &value) { return M5.In_I2C.readRegister(0x6e, reg, &value, 1, 100000); },
      [](uint8_t reg, uint8_t value) { return M5.In_I2C.writeRegister(0x6e, reg, &value, 1, 100000); });
  Serial.printf("[tc.power] accidental_press_protection=%u\n", powerButtonProtected);
#endif
#if TOUCH_CARD_PAPER_MONO
  tc::initPaperFont(paperFontData);
#endif
  if (psramFound())
    heap_caps_malloc_extmem_enable(16384);
#if TOUCH_CARD_PAPER_MONO
  M5.Display.setRotation(0);
  M5.Display.setBrightness(64);
#else
  if (M5.Display.height() > M5.Display.width())
    M5.Display.setRotation(1);
#endif
  prefs = settings.begin("touch_card", false);
  tc::uiLanguage = tc::resolveLanguage(prefs ? settings.getUChar("language", 255) : 255);
  Serial.printf("[tc.ui.language] boot=%s preferences=%u\n",
                tc::uiLanguage == tc::Language::English ? "en" : "ja", prefs);
  if (prefs) {
    landscape = paper() && settings.getBool("landscape", false);
    design = settings.getUChar("design", 0);
    if (design > 2)
      design = 0;
    home = settings.getUChar("home", 0);
    if (home > 2)
      home = 0;
    nameSort = settings.getBool("nameSort", false);
    storage.internalOnly = settings.getBool("internalOnly", false);
  }
  setenv("TZ", "UTC0", 1);
  tzset();
  clockValue.begin();
#if TOUCH_CARD_PAPER_MONO
  const uint32_t storageStarted = millis();
#endif
  storage.begin();
  storage.catalog.pageSize = paper() ? 5 : 3;
#if TOUCH_CARD_PAPER_MONO
  Serial.printf("[tc.boot] storage_init_ms=%lu\n", (unsigned long)(millis() - storageStarted));
  const uint32_t firstFrameStarted = millis();
#endif
  render();
#if TOUCH_CARD_PAPER_MONO
  M5.Display.waitDisplay();
  Serial.printf("[tc.boot] first_frame_ms=%lu setup_ms=%lu\n",
                (unsigned long)(millis() - firstFrameStarted),
                (unsigned long)(millis() - bootStarted));
#endif
  if (!powerButtonProtected)
    notice(tc::tr("電源ボタンの誤操作防止を設定できませんでした。", "Could not protect the power button from accidental presses."));
}
void loop() {
  const auto touchRotation = M5.Display.getRotation();
  const auto touchScreen = screen;
  M5.update();
  // Wake controls must be handled before the lock, otherwise a dark PaperMono
  // could never be unlocked. B is reserved for its light on every screen.
  bool displayToggled = paper() ? M5.BtnB.wasClicked() : M5.BtnPWR.wasClicked();
  if (displayToggled) {
    const auto brightness = screenBlanker.toggle(M5.Display.getBrightness());
    M5.Display.setBrightness(brightness);
    Serial.printf("[tc.ui.screen] on=%u brightness=%u\n", !screenBlanker.off, brightness);
#if TOUCH_CARD_PAPER_MONO
    if (tc::hidePaperFooter(true, screenBlanker.off, screen, home)) {
      // Light-off changes only the reserved footer band, not the card/image.
      M5.Display.setEpdMode(epd_mode_t::epd_fastest);
      M5.Display.startWrite();
      tc::clearSleepingPaperFooter(M5.Display, true, screen, home);
      M5.Display.endWrite();
      Serial.println("[tc.ui.light] footer_hidden=1");
    }
#endif
    if (paper() && !screenBlanker.off) {
#if TOUCH_CARD_PAPER_MONO
      // The panel already retains the card and minute updates. Restore only
      // the footer; quality-mode wake would run a multi-phase erase waveform.
      M5.Display.setEpdMode(epd_mode_t::epd_fastest);
      M5.Display.startWrite();
      const bool restored = paperFooterCache.restore(M5.Display);
      M5.Display.endWrite();
      Serial.printf("[tc.ui.light] wake_partial_refresh=1 footer=%u\n", restored);
#endif
    }
  }
  const bool allowInput = screenBlanker.acceptsInput(
      M5.Touch.getCount() != 0 || M5.BtnA.isPressed() || M5.BtnB.isPressed(),
      displayToggled || (paper() && screenBlanker.waitForRelease && M5.Display.displayBusy()));
  bool scanning = storage.catalog.scanning;
  storage.tick(nfc.active);
  if (screen == Screen::Nfc) {
    if (mutual.readyForNext(millis())) {
      mutual.switching = false;
      waitNfc(false, mutual.sender(), true);
    } else if (!mutual.switching && nfc.readyToClose() && nfcProgressRefresh.completionVisible(millis())) {
      bool sent = nfc.sending;
      bool received = !nfc.sending && !nfc.receiver.phone;
      String id = cards.lastReceivedId;
      if (mutual.active) {
        mutual.token = nfc.exchangeToken();
        mutual.completeLeg(millis());
        Serial.printf("[tc.mutual] leg_complete next=%u sent=%u received=%u active=%u\n",
                      mutual.leg, mutual.sent, mutual.received, mutual.active);
        nfc.stop();
        if (mutual.active) {
          nfcUiSender = mutual.sender();
          nfcProgressRefresh.reset();
          message = tc::tr("1枚目は完了しました。役割を切り替えています。そのままタッチしてください", "First card complete. Switching roles. Keep touching.");
          render();
        } else {
          showReceivedCard(id);
        }
      } else {
        nfc.stop();
        if (received)
          showReceivedCard(id);
        else if (sent)
          notice(tc::tr("名刺を渡しました", "Card sent"));
        else
          navigate(Screen::Home);
      }
    } else if (nfc.failed && (!nfc.active || !nfc.completedAt ||
                              millis() - nfc.completedAt > 4000)) {
      String error = (mutual.active && (mutual.sent || mutual.received) ? tc::tr("1枚目は完了しています。\n", "First card completed.\n") : "") + nfc.error;
      nfc.stop();
      notice(error);
    }
  }
  if (scanning && !storage.catalog.scanning &&
      (screen == Screen::Book || screen == Screen::Usage))
    render();
  if (allowInput && M5.Touch.getCount() && touchRotation == M5.Display.getRotation() &&
      touchScreen == screen) {
    auto touch = M5.Touch.getDetail();
    if (touch.wasClicked()) {
      int x = touch.x, y = touch.y;
      if (!paper() || tc::normalizePaperTouch(x, y, M5.Display.width(),
                                             M5.Display.height()))
        tap(x, y);
    }
  }
  if (allowInput && M5.BtnA.wasHold()) {
    if (screen == Screen::Nfc) cancelNfc();
    else { nfc.stop(); navigate(screen == Screen::Home ? Screen::Menu : Screen::Home); }
  } else if (allowInput && M5.BtnA.wasClicked() && !labels.empty()) {
    moveFocus();
  }
  if (!paper() && allowInput && M5.BtnB.wasClicked()) {
    if (screen == Screen::Nfc) {
      cancelNfc();
    } else if (!labels.empty())
      activate(focus);
    else
      navigate(Screen::Menu);
  }
  // Handle touch/buttons before starting the next synchronous NFC operation.
  nfc.tick();
  if (screen == Screen::Nfc) transferProgress();
  if ((paper() || !nfc.active) && millis() - lastClock >= 1000) {
    lastClock = millis();
    auto now = clockValue.localTime();
    int minute = now.valid ? now.value.tm_yday * 1440 + now.value.tm_hour * 60 +
                                 now.value.tm_min
                           : int(millis() / 60000) + 600000;
    if (!tc::minuteStatusDue(millis(), lastStatusUpdate, minute, lastMinute,
                            paper() && M5.Display.displayBusy())) {
      delay(5);
      return;
    }
    lastMinute = minute;
    lastStatusUpdate = millis();
    int day = now.valid ? now.value.tm_yday : -1;
    if (day != lastDay) {
      if (!paper()) lastDay = day;
      if (!paper() && (screen == Screen::Month || (screen == Screen::Home && home == 1))) {
        render();
        delay(5);
        return;
      }
    }
#if TOUCH_CARD_PAPER_MONO
    // Date/calendar catch-up is regional too, and deferred while dark. Never
    // invoke render() from a PaperMono minute update, even across midnight.
    if (day != lastDay && !screenBlanker.off) {
      refreshPaperDate(now.value, now.valid);
      lastDay = day;
    }
#endif
    if (!(screen == Screen::Home && home == 2)) {
#if TOUCH_CARD_PAPER_MONO
      M5.Display.setEpdMode(epd_mode_t::epd_fastest);
      tc::setPaperFontMonochrome(true);
#endif
      M5.Display.startWrite();
      if (screen == Screen::Home && home == 1)
        clockDashboard(false);
      else
        status();
      M5.Display.endWrite();
      // Keep the photograph out of the monochrome update's bounding box:
      // the PaperMono clock and battery are above the photo, in separate regions.
      if (screen == Screen::Home && home == 1 &&
          dashboardBattery != M5.Power.getBatteryLevel()) {
        M5.Display.startWrite();
        dashboardBatteryStatus();
        M5.Display.endWrite();
      }
      if (paper()) Serial.printf("[tc.ui.minute] partial=1 light_off=%u\n", screenBlanker.off);
    }
  }
  // Keep emulation responsive during a touch; preserve idle power behaviour.
  delay(nfc.active ? 1 : 5);
}
