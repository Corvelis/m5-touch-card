#pragma once
#include <cstdint>
#include <string>

namespace tc {
// Reversible lower-case base32: all 128 ID bits survive, including on
// case-insensitive FAT media. 26 characters + longest suffix (.del) = 30.
inline std::string cardStem(const std::string &hex) {
  static constexpr char alphabet[] = "abcdefghijklmnopqrstuvwxyz234567";
  if (hex.size() != 32 || hex.find_first_not_of("0123456789abcdef") != std::string::npos)
    return hex;
  std::string out;
  uint32_t buffer = 0;
  unsigned bits = 0;
  for (char c : hex) {
    buffer = (buffer << 4) | unsigned(c <= '9' ? c - '0' : c - 'a' + 10);
    bits += 4;
    if (bits >= 5) {
      bits -= 5;
      out += alphabet[(buffer >> bits) & 31];
    }
  }
  if (bits) out += alphabet[(buffer << (5 - bits)) & 31];
  return out;
}
inline std::string cardKey(const std::string &stem) {
  if (stem.size() == 32 && stem.find_first_not_of("0123456789abcdef") == std::string::npos)
    return stem;
  if (stem.size() != 26) return {};
  std::string out;
  uint32_t buffer = 0;
  unsigned bits = 0;
  for (char c : stem) {
    unsigned value;
    if (c >= 'a' && c <= 'z') value = c - 'a';
    else if (c >= '2' && c <= '7') value = c - '2' + 26;
    else return {};
    buffer = (buffer << 5) | value;
    bits += 5;
    while (bits >= 4) {
      bits -= 4;
      out += "0123456789abcdef"[(buffer >> bits) & 15];
    }
  }
  // Reject non-canonical padding; aliases must never create duplicate cards.
  return out.size() == 32 && (buffer & 3) == 0 ? out : std::string{};
}
} // namespace tc
