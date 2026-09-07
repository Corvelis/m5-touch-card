#pragma once
#include <cstddef>
#include <cstdint>

namespace tc {
// ISO/IEC 14443-A CRC, least-significant byte first on the wire.
inline uint16_t nfcACrc(const uint8_t *data, size_t size) {
  uint16_t crc = 0x6363;
  while (size--) {
    crc ^= *data++;
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1) ? 0x8408 : 0);
  }
  return crc;
}

// UnitST25R3916::nfcaReceive returns the raw FIFO length, including CRC_A.
// Validate it before exposing the application payload length. A malformed or
// truncated frame must be retried, not mistaken for a different exchange peer.
inline bool nfcAPayloadSize(const uint8_t *frame, uint16_t &size) {
  if (!frame || size < 2) return false;
  const uint16_t payloadSize = size - 2;
  const uint16_t received = uint16_t(frame[payloadSize]) |
                            (uint16_t(frame[payloadSize + 1]) << 8);
  if (nfcACrc(frame, payloadSize) != received) return false;
  size = payloadSize;
  return true;
}
} // namespace tc
