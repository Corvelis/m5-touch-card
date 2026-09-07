#include "PaperFont.h"
#include "GlyphBitmapCache.h"
#if TOUCH_CARD_PAPER_MONO || defined(TOUCH_CARD_HOST)
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "vendor/stb_truetype.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace tc {
namespace {
stbtt_fontinfo font;
bool ready = false;
bool monochrome = false;
GlyphBitmapCache glyphCache;
}
const PaperFont paper16(16), paper24(24), paper28(28), paper32(32), paper44(44),
                paper52(52), paper56(56), paper96(96);
bool initPaperFont(const unsigned char *data) {
  glyphCache.clear();
  return ready = stbtt_InitFont(&font, data, 0) != 0;
}
void setPaperFontMonochrome(bool enabled) { monochrome = enabled; }
void PaperFont::getDefaultMetric(lgfx::FontMetrics *m) const {
  *m = {};
  m->height = m->y_advance = px_;
  m->baseline = std::lround(px_ * .88f);
}
bool PaperFont::updateFontMetric(lgfx::FontMetrics *m, uint16_t c) const {
  getDefaultMetric(m);
  if (!ready) return false;
  int advance, bearing;
  stbtt_GetCodepointHMetrics(&font, c, &advance, &bearing);
  m->width = m->x_advance = std::lround(
      advance * stbtt_ScaleForMappingEmToPixels(&font, px_));
  return true;
}
size_t PaperFont::drawChar(lgfx::LGFXBase *gfx, int32_t x, int32_t y, uint16_t c,
                           const lgfx::TextStyle *style, lgfx::FontMetrics *m,
                           int32_t &filledX) const {
  if (!ready) return 0;
  const float sx = stbtt_ScaleForMappingEmToPixels(&font, px_ * style->size_x);
  const float sy = stbtt_ScaleForMappingEmToPixels(&font, px_ * style->size_y);
  int x0, y0, x1, y1;
  stbtt_GetCodepointBitmapBox(&font, c, sx, sy, &x0, &y0, &x1, &y1);
  const int w = x1 - x0, h = y1 - y0;
  const int advance = std::lround(m->x_advance * style->size_x);
  if (w <= 0 || h <= 0 || w > 256 || h > 256) return advance;
  const auto &bitmap = glyphCache.get(c, sx, sy, size_t(w * h), [&](unsigned char *pixels) {
    stbtt_MakeCodepointBitmap(&font, pixels, w, h, w, sx, sy, c);
  });
  const int top = y + std::lround(m->baseline * style->size_y) + y0;
  // Four exact luminance levels: no ordered dithering on text edges.
  uint32_t palette[4];
  for (int i = 0; i < 4; ++i) {
    uint32_t color = 0;
    for (int shift : {0, 8, 16}) {
      int bg = (style->back_rgb888 >> shift) & 255;
      int fg = (style->fore_rgb888 >> shift) & 255;
      color |= uint32_t((bg * (3 - i) + fg * i) / 3) << shift;
    }
#if !defined(TOUCH_CARD_HOST)
    // SSD1677 adds a +/-30 ordered dither before shifting by 6 bits.
    // 96 and 160 are the only stable middle-level centres; 85/170 dither.
    const int gray = color & 255;
    const int carriers[] = {0, 96, 160, 255};
    const int level = std::min(3, (gray + 42) / 85);
    color = uint32_t(carriers[level]) * 0x010101u;
#endif
    if (monochrome) color = (color & 255) >= 128 ? 0xFFFFFFu : 0x000000u;
    palette[i] = color;
  }
  gfx->startWrite();
  for (int row = 0; row < h; ++row) {
    for (int col = 0; col < w;) {
      int start = col;
      int level = (bitmap[row * w + col] * 3 + 127) / 255;
      while (++col < w && (bitmap[row * w + col] * 3 + 127) / 255 == level) {}
      if (level)
        gfx->drawFastHLine(x + x0 + start, top + row, col - start, palette[level]);
    }
  }
  gfx->endWrite();
  filledX = std::max(filledX, x + advance);
  return advance;
}
} // namespace tc
#endif
