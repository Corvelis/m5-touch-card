#pragma once
#include <cstdint>
namespace tc {
constexpr uint8_t MutualCapability = 32;
// Local workflow only. A leg completes only after the NFC layer has confirmed
// durable storage and completed its FINISH handshake/grace period.
struct MutualExchange {
  bool active = false, firstSender = false, switching = false;
  bool sent = false, received = false;
  uint8_t leg = 0;
  uint32_t switchedAt = 0;
  uint64_t token = 0;
  void reset() { *this = {}; }
  void start(bool sender) { reset(); active = true; firstSender = sender; leg = 1; }
  bool sender() const { return leg == 1 ? firstSender : !firstSender; }
  void completeLeg(uint32_t now) {
    if (!active || switching) return;
    if (sender()) sent = true; else received = true;
    if (leg == 1) { leg = 2; switching = true; switchedAt = now; }
    else active = false;
  }
  bool readyForNext(uint32_t now) const {
    // The old reader disables its field and becomes a listener first. The
    // new reader waits without blocking touch; discovery also tolerates delay.
    return active && switching && uint32_t(now - switchedAt) >= (sender() ? 1200U : 0U);
  }
};
} // namespace tc
