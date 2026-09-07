#pragma once
#include <algorithm>
#include <cstdint>

namespace tc {
enum class TransferStage { Waiting, Connecting, Transferring, Reconnecting, Saving, Complete, Failed };
struct TransferProgress {
  TransferStage stage = TransferStage::Waiting;
  uint32_t transferred = 0, total = 0;
  unsigned percent() const {
    return stage == TransferStage::Complete ? 100 : total
        ? unsigned(std::min<uint64_t>(100, uint64_t(transferred) * 100 / total)) : 0;
  }
};
// No animation while idle. E-paper updates only on meaningful state/20% steps,
// never while its panel is busy; LCD can show finer progress.
class ProgressRefresh {
 public:
  bool initialized = false;
  TransferProgress shown;
  uint32_t updatedAt = 0, completedShownAt = 0;
  void reset() { initialized = false; completedShownAt = 0; }
  bool needed(const TransferProgress &next, uint32_t now, bool paper, bool busy) const {
    if (busy) return false;
    if (!initialized) return true;
    if (next.stage != shown.stage) {
      if (next.stage == TransferStage::Complete || next.stage == TransferStage::Failed ||
          next.stage == TransferStage::Saving || shown.stage == TransferStage::Waiting) return true;
      return uint32_t(now - updatedAt) >= (paper ? 250U : 80U);
    }
    const unsigned step = paper ? 20 : 5;
    return next.stage == TransferStage::Transferring &&
           next.percent() / step != shown.percent() / step &&
           uint32_t(now - updatedAt) >= (paper ? 500U : 120U);
  }
  void record(const TransferProgress &next, uint32_t now) {
    if (next.stage == TransferStage::Complete &&
        (!initialized || shown.stage != TransferStage::Complete)) completedShownAt = now;
    initialized = true; shown = next; updatedAt = now;
  }
  bool completionVisible(uint32_t now) const {
    return initialized && shown.stage == TransferStage::Complete &&
           uint32_t(now - completedShownAt) >= 600;
  }
};
} // namespace tc
