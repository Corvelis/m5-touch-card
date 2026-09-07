#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include "MutualExchange.h"
namespace tc {
constexpr uint32_t MaxPayload = 524288;
enum Status : uint8_t {
  Ok = 0,
  Accepted = 1,
  Busy = 2,
  Conflict = 3,
  BadMagic = 4,
  BadVersion = 5,
  Unknown = 6,
  BadLength = 7,
  TooLarge = 8,
  BadImage = 9,
  BadOffset = 10,
  Mismatch = 11,
  BadCrc = 12,
  InvalidData = 13,
  Unsupported = 14,
  NotFound = 15,
  StorageError = 16,
  Receiving = 17,
  Verifying = 18,
  Stored = 19,
  NoSpace = 22,
  MediaLost = 23,
  WrongSession = 24
};
inline uint16_t le16(const uint8_t *p) { return p[0] | uint16_t(p[1]) << 8; }
inline uint32_t le32(const uint8_t *p) {
  return le16(p) | uint32_t(le16(p + 2)) << 16;
}
inline void put16(uint8_t *p, uint16_t n) {
  p[0] = n;
  p[1] = n >> 8;
}
inline void put32(uint8_t *p, uint32_t n) {
  put16(p, n);
  put16(p + 2, n >> 16);
}
inline uint32_t crc(const uint8_t *p, size_t n) {
  uint32_t v = ~0U;
  while (n--) {
    v ^= *p++;
    for (int b = 0; b < 8; b++)
      v = (v >> 1) ^ (0xEDB88320U & (0U - (v & 1)));
  }
  return ~v;
}
// No filesystem calls in handle(). beginSession allocates before enabling RF.
class Receiver {
public:
  std::vector<uint8_t> buffer;
  uint32_t id = 0, size = 0, expectedCrc = 0, offset = 0, lastActivity = 0;
  uint8_t target = 0, device = 1, encoding = 1;
  uint8_t mutualLeg = 0;
  uint64_t mutualToken = 0;
  bool mutualPaired = false;
  Status status = Ok;
  bool phone = true, active = false, commitPending = false,
       clockPending = false, finishRequested = false;
  uint64_t unixSeconds = 0;
  int16_t utcOffset = 0;
  void beginSession(bool update, uint8_t board, uint8_t leg = 0, uint64_t token = 0) {
    buffer.resize(MaxPayload);
    phone = update;
    mutualLeg = update ? 0 : leg;
    mutualToken = token;
    mutualPaired = false;
    device = board;
    active = true;
    id = size = offset = 0;
    target = 0;
    encoding = 1;
    finishRequested = false;
    status = Ok;
    commitPending = clockPending = false;
  }
  void cancel() {
    mutualLeg = 0; mutualToken = 0; mutualPaired = false;
    active = false;
    commitPending = clockPending = false;
    finishRequested = false;
    status = Ok;
    id = size = offset = 0;
  }
  size_t handle(const uint8_t *p, size_t n, uint8_t *out, uint32_t now) {
    if (!active || n < 4 || p[0] != 0x54 || p[1] != 0x43)
      return 0;
    uint8_t cmd = p[3];
    Status reply = Ok;
    size_t len = 13;
    if (p[2] != 2)
      reply = BadVersion;
    else if (n > 253)
      reply = BadLength;
    else if (cmd == 1) {
      if (n != 4)
        reply = BadLength;
      else {
        put16(out + 13, 255);
        put16(out + 15, 253);
        put16(out + 17, 240);
        put32(out + 19, MaxPayload);
        out[23] = 31; // JSON, clock, typed updates, compact cards, FINISH
        out[24] = device;
        out[25] = phone ? 1 : 2;
        len = 26;
        if (mutualLeg) { out[23] |= MutualCapability; out[26] = mutualLeg; len = 27; }
      }
    } else if (cmd == 9) {
      if (n != 13) reply = BadLength;
      else if (!mutualLeg || phone || p[12] != mutualLeg) reply = WrongSession;
      else {
        const uint64_t token = uint64_t(le32(p + 4)) | (uint64_t(le32(p + 8)) << 32);
        if (!token) reply = InvalidData;
        else if ((mutualToken && token != mutualToken) || (mutualLeg == 2 && !mutualToken))
          reply = Conflict;
        else {
          mutualToken = token; mutualPaired = true;
          put32(out + 13, uint32_t(token)); put32(out + 17, uint32_t(token >> 32));
          out[21] = mutualLeg; len = 22;
        }
      }
    } else if (cmd == 2) {
      if (n != 23 || !le32(p + 4) || p[8] > 1 || le16(p + 11) ||
          le16(p + 13))
        reply = BadLength;
      else if (p[9] < 1 || p[9] > 5 || (phone ? p[9] == 5 : p[9] != 5))
        reply = WrongSession;
      else if (mutualLeg && !mutualPaired)
        reply = WrongSession;
      else if (p[10] != 1 && !(p[10] == 2 && p[9] == 5))
        reply = Unsupported;
      else if (!le32(p + 15) || le32(p + 15) > MaxPayload)
        reply = TooLarge;
      else if (commitPending || clockPending)
        reply = Busy;
      else if (id == le32(p + 4)) {
        reply = (target == p[9] && encoding == p[10] && size == le32(p + 15) &&
                 expectedCrc == le32(p + 19))
                    ? status
                    : Conflict;
        if (reply == Ok) {
          status = Receiving;
          reply = Accepted;
        }
      } else if (id && status == Receiving && p[8] != 1)
        reply = Conflict;
      else {
        id = le32(p + 4);
        target = p[9];
        encoding = p[10];
        finishRequested = false;
        size = le32(p + 15);
        expectedCrc = le32(p + 19);
        offset = 0;
        status = Receiving;
        reply = Accepted;
      }
    } else if (cmd == 7) {
      if (!phone)
        reply = WrongSession;
      else if (n != 15 || p[14])
        reply = BadLength;
      else if (commitPending || status == Receiving)
        reply = Busy;
      else {
        uint64_t seconds = le32(p + 4) | (uint64_t(le32(p + 8)) << 32);
        int16_t zone = static_cast<int16_t>(le16(p + 12));
        if (seconds < 1672531200ULL || seconds >= 4102444800ULL ||
            zone < -840 || zone > 840)
          reply = InvalidData;
        else if (clockPending)
          reply = Busy;
        else if (unixSeconds == seconds && utcOffset == zone &&
                 status == Stored)
          reply = Ok;
        else {
          unixSeconds = seconds;
          utcOffset = zone;
          clockPending = true;
          reply = Busy;
        }
      }
    } else if (n < 8)
      reply = BadLength;
    else if (!id || le32(p + 4) != id)
      reply = NotFound;
    else if (cmd == 3) {
      if (n < 14 || n != size_t(13 + p[12]) || !p[12] || p[12] > 240)
        reply = BadLength;
      else if (status != Receiving)
        reply = status;
      else {
        uint32_t pos = le32(p + 8), count = p[12];
        if (pos > offset || count > size || pos > size - count)
          reply = BadOffset;
        else if (pos < offset)
          reply = (pos + count <= offset &&
                   !memcmp(buffer.data() + pos, p + 13, count))
                      ? Receiving
                      : Mismatch;
        else {
          memcpy(buffer.data() + offset, p + 13, count);
          offset += count;
          reply = Receiving;
        }
      }
    } else if (cmd == 4)
      reply = n == 8 ? status : BadLength;
    else if (cmd == 5) {
      if (n != 12)
        reply = BadLength;
      else if (le32(p + 8) != expectedCrc)
        reply = BadCrc;
      else if (status == Stored || status == Verifying)
        reply = status;
      else if (status != Receiving)
        reply = status;
      else if (offset != size)
        reply = BadOffset;
      else {
        status = Verifying;
        commitPending = true;
        reply = Verifying;
      }
    } else if (cmd == 8) {
      if (n != 8) reply = BadLength;
      else if (phone) reply = WrongSession;
      else {
        reply = status;
        if (status == Stored) finishRequested = true;
      }
    } else if (cmd == 6) {
      if (n != 8)
        reply = BadLength;
      else if (commitPending)
        reply = Busy;
      else {
        status = Ok;
        finishRequested = false;
        offset = 0;
        reply = Ok;
      }
    } else
      reply = Unknown;
    lastActivity = now;
    out[0] = 0x54;
    out[1] = 0x43;
    out[2] = 2;
    out[3] = cmd | 0x80;
    out[4] = reply;
    put32(out + 5, id);
    put32(out + 9, offset);
    return len;
  }
};
} // namespace tc
