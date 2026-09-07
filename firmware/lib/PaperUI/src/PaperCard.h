#pragma once
#include "CardLayout.h"
#include "PaperFont.h"
#include <lgfx/utility/lgfx_qrcode.h>
#include <algorithm>
#include <string>
#include <vector>

namespace tc {
struct PaperProfile {
  const char *name, *account, *email, *comment, *url;
};
inline bool drawPaperQr(lgfx::LGFXBase &d, const char *url, CardRect box) {
  if (!url || !*url) return false;
  QRCode code;
  for (int version = 1; version <= 20; ++version) {
    std::vector<uint8_t> data(lgfx_qrcode_getBufferSize(version));
    if (lgfx_qrcode_initText(&code, data.data(), version, 0, url)) continue;
    const int cell = box.w / (code.size + 8);
    if (cell < 1) return false;
    const int offset = (box.w - code.size * cell) / 2;
    d.fillRect(box.x, box.y, box.w, box.h, 0xFFFFFFu);
    for (int y = 0; y < code.size; ++y) for (int x = 0; x < code.size; ++x) {
      const int bit = y * code.size + x;
      if (data[bit >> 3] & (1u << (7 - (bit & 7))))
        d.fillRect(box.x + offset + x * cell, box.y + offset + y * cell,
                    cell, cell, 0x000000u);
    }
    return true;
  }
  return false;
}
inline std::vector<std::string> paperLines(lgfx::LGFXBase &d,
                                          const std::string &value, int width) {
  std::vector<std::string> result;
  std::string line;
  for (size_t pos = 0; pos < value.size();) {
    const unsigned char c = value[pos];
    const size_t len = c < 128 ? 1 : c < 224 ? 2 : c < 240 ? 3 : 4;
    auto next = value.substr(pos, len);
    pos += len;
    if (next == "\n") { result.push_back(line); line.clear(); continue; }
    if (!line.empty() && d.textWidth((line + next).c_str()) > width) {
      // Keep Latin names/addresses together when a word boundary is available.
      const auto space = line.find_last_of(' ');
      if (space != std::string::npos && space > 0) {
        result.push_back(line.substr(0, space));
        line = line.substr(space + 1);
      } else {
        result.push_back(line); line.clear();
      }
    }
    if (!line.empty() || next != " ") line += next;
  }
  if (!line.empty()) result.push_back(line);
  return result;
}
inline void paperTextBox(lgfx::LGFXBase &d, const char *value, CardRect box,
                          const lgfx::IFont &font, int step, bool centered = false,
                          uint32_t fg = 0x000000, uint32_t bg = 0xFFFFFF) {
  if (!value || !*value || box.w <= 0 || box.h <= 0) return;
  d.setFont(&font); d.setTextSize(1); d.setTextColor(fg, bg);
  auto lines = paperLines(d, value, box.w);
  const int maximum = std::max(1, box.h / step);
  if (int(lines.size()) > maximum) {
    lines.resize(maximum);
    auto &last = lines.back();
    while (!last.empty() && d.textWidth((last + "…").c_str()) > box.w) {
      size_t cut = last.size() - 1;
      while (cut && (static_cast<unsigned char>(last[cut]) & 0xC0) == 0x80) --cut;
      last.resize(cut);
    }
    last += "…";
  }
  d.setClipRect(box.x, box.y, box.w, box.h);
  for (size_t i = 0; i < lines.size(); ++i) {
    int x = box.x + (centered ? (box.w - d.textWidth(lines[i].c_str())) / 2 : 0);
    d.drawString(lines[i].c_str(), x, box.y + i * step);
  }
  d.clearClipRect();
}
template <typename Avatar>
void drawPaperCard(lgfx::LGFXBase &d, const PaperProfile &p, uint8_t design,
                   bool horizontal, Avatar avatar) {
  const auto l = paperCardLayout(design, horizontal);
  const bool contrast = design == 1;
  if (l.band.w) d.fillRect(l.band.x, l.band.y, l.band.w, l.band.h, 0x000000);
  avatar(l.avatar, contrast ? 0x000000 : 0xFFFFFF);
  paperTextBox(d, p.name, l.name, l.namePx == 52 ? paper52 : paper56,
               l.namePx + 6, l.centered, contrast ? 0xFFFFFF : 0x000000,
               contrast ? 0x000000 : 0xFFFFFF);
  if (l.accent.w)
    d.fillRect(l.accent.x, l.accent.y, l.accent.w, l.accent.h, 0x000000);
  const bool darkAccount = contrast && !horizontal;
  paperTextBox(d, p.account, l.account, paper32, 38, l.centered,
               darkAccount ? 0xFFFFFF : 0x000000,
               darkAccount ? 0x000000 : 0xFFFFFF);
  paperTextBox(d, p.email, l.email, paper28, 34, l.centered);
  paperTextBox(d, p.comment, l.comment, paper28, 34, l.centered);
  if (p.url && *p.url)
    drawPaperQr(d, p.url, l.qr);
}
inline void drawPaperMenuRow(lgfx::LGFXBase &d, const char *label, int top,
                             int height, bool selected) {
  // Stable divider + separate focus rail, avoiding large inverted areas.
  d.drawFastHLine(32, top + height - 1, d.width() - 64, 0x000000);
  if (selected) d.fillRoundRect(16, top + 24, 4, height - 48, 2, 0x000000);
  paperTextBox(d, label, {32, top + (height - 32) / 2, d.width() - 96, 40},
               paper32, 38);
  const int x = d.width() - 40, y = top + height / 2;
  d.drawLine(x - 5, y - 7, x + 2, y, 0x000000);
  d.drawLine(x + 2, y, x - 5, y + 7, 0x000000);
}
template <typename Avatar>
void drawPaperBookEntry(lgfx::LGFXBase &d, const char *name, const char *account,
                        int top, bool selected, Avatar avatar) {
  d.drawFastHLine(32, top + 119, d.width() - 64, 0x000000u);
  if (selected) d.fillRoundRect(16, top + 24, 4, 72, 2, 0x000000u);
  avatar(CardRect{32, top + 16, 88, 88});
  paperTextBox(d, name, {144, top + 20, d.width() - 168, 40}, paper32, 38);
  paperTextBox(d, account, {144, top + 68, d.width() - 168, 30}, paper24, 28);
}
} // namespace tc
