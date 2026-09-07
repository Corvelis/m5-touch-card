#pragma once
#include <cstdint>
#include <list>
#include <vector>

namespace tc {
// Bounded, in-memory cache of trusted font rasters. Colours and monochrome
// mode are applied by the renderer, so changing them needs no invalidation.
class GlyphBitmapCache {
  struct Glyph {
    uint16_t code;
    float px, py;
    std::vector<unsigned char> bitmap;
  };
  std::list<Glyph> glyphs;
  size_t used = 0;
 public:
  static constexpr size_t maxBytes = 128 * 1024, maxEntries = 64;
  void clear() { glyphs.clear(); used = 0; }
  size_t bytes() const { return used; }
  size_t count() const { return glyphs.size(); }
  template <class Rasterize>
  const std::vector<unsigned char> &get(uint16_t code, float px, float py,
                                       size_t size, Rasterize rasterize) {
    for (auto i = glyphs.begin(); i != glyphs.end(); ++i)
      if (i->code == code && i->px == px && i->py == py) {
        glyphs.splice(glyphs.begin(), glyphs, i);
        return glyphs.front().bitmap;
      }
    // Caller bounds each glyph to 256 x 256, smaller than maxBytes.
    while (!glyphs.empty() && (used + size > maxBytes || glyphs.size() >= maxEntries)) {
      used -= glyphs.back().bitmap.size(); glyphs.pop_back();
    }
    glyphs.push_front({code, px, py, std::vector<unsigned char>(size)});
    used += size;
    rasterize(glyphs.front().bitmap.data());
    return glyphs.front().bitmap;
  }
};
} // namespace tc
