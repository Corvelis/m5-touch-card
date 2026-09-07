#pragma once
#include "CardStore.h"

namespace tc {
constexpr uint8_t JsonEncoding = 1, CompactCardEncoding = 2;
constexpr uint8_t CompactCardCapability = 8, FinishCapability = 16;
constexpr size_t ExchangeAvatarBudget = 8 * 1024;
constexpr size_t MaxCardMetadata = 8192;

inline std::string base64Encode(const Bytes &bytes) {
  static const char alphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((bytes.size() + 2) / 3 * 4);
  for (size_t i = 0; i < bytes.size(); i += 3) {
    uint32_t v = uint32_t(bytes[i]) << 16;
    if (i + 1 < bytes.size()) v |= uint32_t(bytes[i + 1]) << 8;
    if (i + 2 < bytes.size()) v |= bytes[i + 2];
    out += alphabet[v >> 18];
    out += alphabet[(v >> 12) & 63];
    out += i + 1 < bytes.size() ? alphabet[(v >> 6) & 63] : '=';
    out += i + 2 < bytes.size() ? alphabet[v & 63] : '=';
  }
  return out;
}
inline bool base64Decode(const std::string &text, Bytes &out) {
  out.clear();
  if (text.empty() || text.size() % 4 || text.size() > 349528) return false;
  auto digit = [](char c) -> int {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    return c == '+' ? 62 : c == '/' ? 63 : -1;
  };
  for (size_t i = 0; i < text.size(); i += 4) {
    int a = digit(text[i]), b = digit(text[i + 1]);
    bool pad2 = text[i + 2] == '=', pad3 = text[i + 3] == '=';
    int c = pad2 ? 0 : digit(text[i + 2]);
    int d = pad3 ? 0 : digit(text[i + 3]);
    if (a < 0 || b < 0 || c < 0 || d < 0 || (pad2 && !pad3) ||
        ((pad2 || pad3) && i + 4 != text.size()) ||
        (pad2 && (b & 15)) || (pad3 && !pad2 && (c & 3))) return false;
    uint32_t v = uint32_t(a) << 18 | uint32_t(b) << 12 | uint32_t(c) << 6 | d;
    out.push_back(v >> 16);
    if (!pad2) out.push_back(v >> 8);
    if (!pad3) out.push_back(v);
  }
  return out.size() <= 262144;
}

// One stable avatar is selected before negotiating the transport. Legacy and
// compact peers receive identical normalized card content at the same revision.
inline void exchangeCard(const JsonDocument &own, JsonDocument &card) {
  card.clear();
  card["target"] = 5;
  card["profile"] = own["profile"];
  card["avatar"] = own["exchangeAvatar"].isNull() ? own["avatar"]
                                                   : own["exchangeAvatar"];
}

// Encoding 2: TCJ1, metadata length:u32, JPEG length:u32, JSON metadata, raw JPEG.
// Only transport changes. On disk the established normalized JSON stays intact.
inline bool packCard(const JsonDocument &card, Bytes &out) {
  if (card["target"] != 5 || !profileValid(card["profile"])) return false;
  JsonDocument metadata;
  metadata["target"] = 5;
  metadata["profile"] = card["profile"];
  Bytes jpeg;
  if (card["avatar"].isNull()) metadata["avatar"] = nullptr;
  else {
    auto avatar = card["avatar"];
    if (!avatar["jpeg"].is<std::string>() ||
        !base64Decode(avatar["jpeg"].as<std::string>(), jpeg) ||
        !avatar["width"].is<int>() || !avatar["height"].is<int>() ||
        avatar["width"].as<int>() < 1 || avatar["width"].as<int>() > 256 ||
        avatar["height"].as<int>() < 1 || avatar["height"].as<int>() > 256)
      return false;
    metadata["avatar"]["width"] = avatar["width"];
    metadata["avatar"]["height"] = avatar["height"];
  }
  auto json = encode(metadata);
  if (json.size() > MaxCardMetadata) return false;
  out.assign(12, 0);
  memcpy(out.data(), "TCJ1", 4);
  put32(out.data() + 4, json.size());
  put32(out.data() + 8, jpeg.size());
  out.insert(out.end(), json.begin(), json.end());
  out.insert(out.end(), jpeg.begin(), jpeg.end());
  return out.size() <= MaxPayload;
}
inline bool unpackCard(const Bytes &wire, Bytes &json) {
  if (wire.size() < 12 || memcmp(wire.data(), "TCJ1", 4)) return false;
  size_t metadataSize = le32(wire.data() + 4), jpegSize = le32(wire.data() + 8);
  if (!metadataSize || metadataSize > MaxCardMetadata || jpegSize > 262144 ||
      metadataSize + jpegSize != wire.size() - 12) return false;
  JsonDocument card;
  if (deserializeJson(card, wire.data() + 12, metadataSize) ||
      card["target"] != 5 || !profileValid(card["profile"])) return false;
  if (jpegSize) {
    auto avatar = card["avatar"];
    if (!avatar.is<JsonObject>() || avatar.size() != 2 ||
        !avatar["width"].is<int>() || !avatar["height"].is<int>()) return false;
    int w = avatar["width"], h = avatar["height"];
    if (w < 1 || w > 256 || h < 1 || h > 256) return false;
    Bytes jpeg(wire.begin() + 12 + metadataSize, wire.end());
    // The normal service validates baseline JPEG and actual dimensions later.
    card.remove("avatar");
    card["avatar"]["jpeg"] = base64Encode(jpeg);
    card["avatar"]["width"] = w;
    card["avatar"]["height"] = h;
  } else if (!card["avatar"].isNull()) return false;
  json = encode(card);
  return json.size() <= MaxPayload;
}

// FINISH can shorten only the UI grace period, never verification/persistence.
inline bool exchangeUiReady(bool sending, bool finished, uint32_t savedElapsedMs,
                            uint32_t finishElapsedMs) {
  return sending || (finished && finishElapsedMs >= 20) || savedElapsedMs >= 4000;
}
} // namespace tc
