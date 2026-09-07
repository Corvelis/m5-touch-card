#include "GlyphBitmapCache.h"
#include <cassert>
#include <iostream>

int main() {
  tc::GlyphBitmapCache cache;
  unsigned renders = 0;
  auto raster = [&](unsigned char *p) { ++renders; p[0] = 42; };
  assert(cache.get('0', 1, 1, 100, raster)[0] == 42);
  assert(cache.get('0', 1, 1, 100, raster)[0] == 42 && renders == 1);
  cache.get('0', 2, 1, 200, raster);
  cache.get('0', 1, 2, 200, raster);
  assert(renders == 3);
  for (unsigned i = 0; i < 200; ++i) {
    cache.get(i, 1, 1, 65536, raster);
    assert(cache.bytes() <= cache.maxBytes && cache.count() <= cache.maxEntries);
  }
  unsigned before = renders;
  cache.get('0', 1, 1, 100, raster);
  assert(renders == before + 1); // Evicted glyph is rasterized again.
  for (unsigned i = 0; i < 200; ++i) {
    cache.get(i, 1, 1, 1, raster);
    assert(cache.count() <= cache.maxEntries);
  }
  cache.clear();
  assert(cache.bytes() == 0 && cache.count() == 0);
  before = renders;
  cache.get(199, 1, 1, 1, raster);
  assert(renders == before + 1);
  std::cout << "Bounded glyph cache: PASS\n";
}
