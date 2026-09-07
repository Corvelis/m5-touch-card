#pragma once
#include <M5GFX.h>

namespace tc {
// Only the compiled-in, trusted font may be initialized. NFC never loads fonts.
bool initPaperFont(const unsigned char *trustedFont);
void setPaperFontMonochrome(bool enabled);
class PaperFont final : public lgfx::IFont {
public:
  explicit constexpr PaperFont(int px) : px_(px) {}
  font_type_t getType() const override { return ft_ttf; }
  void getDefaultMetric(lgfx::FontMetrics *m) const override;
  bool updateFontMetric(lgfx::FontMetrics *m, uint16_t c) const override;
  size_t drawChar(lgfx::LGFXBase *gfx, int32_t x, int32_t y, uint16_t c,
                  const lgfx::TextStyle *style, lgfx::FontMetrics *m,
                  int32_t &filledX) const override;
private:
  int px_;
};
extern const PaperFont paper16, paper24, paper28, paper32, paper44, paper52,
                       paper56, paper96;
} // namespace tc
