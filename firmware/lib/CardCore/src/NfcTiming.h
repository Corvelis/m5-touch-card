#pragma once
#include <cstddef>
#include <cstdint>
namespace tc {
// At 106 kbit/s the largest DATA frame occupies about 22 ms on air. Allow
// additional I2C + listener latency, without the old 500 ms retry penalty.
inline uint32_t cardResponseTimeout(int phase) { return phase == 2 ? 80 : 150; }
// Flash commits may temporarily stop the listener. Don't reset its RF field
// merely because the durable save is still running.
inline bool waitForCardSave(uint32_t elapsed) { return elapsed < 3000; }
inline size_t cardRetryChunk(size_t chunk, unsigned failuresWithoutProgress) {
  if (failuresWithoutProgress >= 12 && chunk > 64) return 64;
  if (failuresWithoutProgress >= 6 && chunk > 128) return 128;
  return chunk;
}
} // namespace tc
