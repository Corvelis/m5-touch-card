#pragma once
#include "CardWire.h"
#include "CardFilename.h"
#include <ArduinoJson.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace tc {
using Bytes = std::vector<uint8_t>;
struct Entry {
  std::string key, name, account;
  uint32_t revision = 0;
  uint64_t sequence = 0, generation = 0;
  uint32_t checksum = 0, size = 0;
  int slot = 0, medium = 0;
};
class Cursor {
public:
  virtual ~Cursor() = default;
  virtual std::string next() = 0;
};
class IO {
public:
  virtual ~IO() = default;
  virtual bool read(const std::string &, Bytes &, size_t) = 0;
  virtual bool write(const std::string &, const Bytes &) = 0;
  virtual bool remove(const std::string &) = 0;
  virtual bool exists(const std::string &) = 0;
  virtual uint64_t fileSize(const std::string &) = 0;
  virtual bool mkdir(const std::string &) = 0;
  virtual uint64_t freeBytes() = 0;
  virtual std::unique_ptr<Cursor> list(const std::string &) = 0;
  bool ready = false, writable = false;
  uint64_t generation = 0;
};
inline Bytes encode(const JsonDocument &doc) {
  std::string s;
  serializeJson(doc, s);
  return Bytes(s.begin(), s.end());
}
inline bool parse(const Bytes &b, JsonDocument &d) {
  return !deserializeJson(d, b.data(), b.size());
}
inline bool keyValid(const std::string &key) {
  if (key == "own" || key == "dashboard" || key == "fullscreen")
    return true;
  return key.size() == 32 &&
         key.find_first_not_of("0123456789abcdef") == std::string::npos;
}
inline bool textValid(const char *s, size_t limit, bool required = false) {
  if (!s || (required && !*s))
    return false;
  size_t count = 0;
  const auto *p = reinterpret_cast<const uint8_t *>(s);
  while (*p) {
    uint32_t code = *p++;
    int extra = 0;
    if (code < 0x80) {
      if (code < 32 || code == 127)
        return false;
    } else if (code >= 0xc2 && code <= 0xdf) {
      code &= 31;
      extra = 1;
    } else if (code >= 0xe0 && code <= 0xef) {
      code &= 15;
      extra = 2;
    } else if (code >= 0xf0 && code <= 0xf4) {
      code &= 7;
      extra = 3;
    } else
      return false;
    int original = extra;
    while (extra--) {
      if ((*p & 0xc0) != 0x80)
        return false;
      code = (code << 6) | (*p++ & 63);
    }
    if ((original == 1 && code < 128) || (original == 2 && code < 2048) ||
        (original == 3 && code < 65536) || code > 0x10ffff ||
        (code >= 0xd800 && code <= 0xdfff))
      return false;
    if (++count > limit)
      return false;
  }
  return true;
}
inline bool profileValid(JsonVariantConst p) {
  if (!p.is<JsonObjectConst>() || !p["id"].is<const char *>() ||
      !keyValid(p["id"].as<std::string>()) ||
      p["id"].as<std::string>().size() != 32 || !p["revision"].is<uint32_t>())
    return false;
  const char *names[] = {"name", "account", "email", "url", "comment"};
  const int limits[] = {80, 80, 254, 512, 40};
  for (int i = 0; i < 5; i++)
    if (!p[names[i]].is<const char *>() ||
        p[names[i]].as<std::string>().size() !=
            strlen(p[names[i]].as<const char *>()) ||
        !textValid(p[names[i]].as<const char *>(), limits[i], i == 0))
      return false;
  std::string url = p["url"].as<std::string>();
  if (url.empty())
    return true;
  size_t host = url.rfind("https://", 0) == 0  ? 8
                : url.rfind("http://", 0) == 0 ? 7
                                               : 0;
  return url.size() <= 512 && host && url.size() > host &&
         url.find(' ') == std::string::npos &&
         url.find_first_of("/?#:", host) != host;
}

// Per-key dual generations. Metadata is published last and checksummed too.
// A delete marker masks both generations before removal begins, preventing
// resurrection.
class Journal {
public:
  IO &io;
  bool legacyNames = false;
  uint64_t freeBeforeSave = 0;
  const char *lastFailure = "none";
  Status storageFailure(const char *stage) {
    lastFailure = stage;
    return StorageError;
  }
  explicit Journal(IO &fs, bool legacy = false) : io(fs), legacyNames(legacy) {}
  std::string path(const std::string &k, const std::string &suffix) const {
    if (legacyNames || k.size() != 32) return "/tc/" + k + suffix;
    return "/tc/" + cardStem(k) + (suffix == ".delete" ? ".del" : suffix);
  }
  bool hasArtifacts(const std::string &key) {
    for (const char *suffix : {".a", ".b", ".ia", ".ib", ".delete"})
      if (io.exists(path(key, suffix))) return true;
    return false;
  }
  bool decodeMetadata(const std::string &key, int slot, const Bytes &raw,
                      Entry &e) {
    JsonDocument d;
    if (!parse(raw, d) || !d["body"].is<std::string>())
      return false;
    std::string body = d["body"].as<std::string>();
    if (!d["crc"].is<uint32_t>() ||
        crc(reinterpret_cast<const uint8_t *>(body.data()), body.size()) !=
            d["crc"].as<uint32_t>())
      return false;
    JsonDocument m;
    if (deserializeJson(m, body) || m["key"] != key ||
        !m["generation"].is<uint64_t>() || !m["size"].is<uint32_t>() ||
        !m["crc"].is<uint32_t>())
      return false;
    e.key = key;
    e.slot = slot;
    e.generation = m["generation"];
    e.size = m["size"];
    e.checksum = m["crc"];
    e.name = m["name"] | "";
    e.account = m["account"] | "";
    e.revision = m["revision"] | 0U;
    e.sequence = m["sequence"] | uint64_t(0);
    return e.generation > 0 && e.size <= MaxPayload && e.size > 0 &&
           textValid(e.name.c_str(), 80) && textValid(e.account.c_str(), 80);
  }
  // Each record carries its own checked index. A missing/corrupt sidecar can be
  // rebuilt without trusting an incomplete write or scanning other directories.
  bool readRecord(const std::string &key, int slot, Bytes &data, Entry &entry,
                  Bytes *index = nullptr) {
    Bytes raw;
    if (!io.read(path(key, slot ? ".b" : ".a"), raw, MaxPayload + 2064) ||
        raw.size() < 16 || memcmp(raw.data(), "TCR2", 4))
      return false;
    uint32_t metaSize = le32(raw.data() + 4),
             payloadSize = le32(raw.data() + 8);
    if (!metaSize || metaSize > 2048 || !payloadSize ||
        payloadSize > MaxPayload ||
        raw.size() != 16ULL + metaSize + payloadSize ||
        le32(raw.data() + 12) != crc(raw.data() + 16, raw.size() - 16))
      return false;
    Bytes metadata(raw.begin() + 16, raw.begin() + 16 + metaSize);
    if (!decodeMetadata(key, slot, metadata, entry) ||
        entry.size != payloadSize ||
        entry.checksum != crc(raw.data() + 16 + metaSize, payloadSize))
      return false;
    data.assign(raw.begin() + 16 + metaSize, raw.end());
    if (index)
      *index = std::move(metadata);
    return true;
  }
  bool metadata(const std::string &key, int slot, Entry &e) {
    if (!keyValid(key) || io.exists(path(key, ".delete")))
      return false;
    Bytes raw;
    auto metaPath = path(key, slot ? ".ib" : ".ia");
    if (io.read(metaPath, raw, 2048) && decodeMetadata(key, slot, raw, e))
      return true;
    Bytes payload, index;
    if (!readRecord(key, slot, payload, e, &index))
      return false;
    if (io.writable)
      io.write(metaPath, index); // Repair is optional for reading.
    return true;
  }
  bool latest(const std::string &key, Entry &e) {
    Bytes verified;
    return load(key, verified, &e);
  }
  bool load(const std::string &key, Bytes &data, Entry *output = nullptr) {
    if (!keyValid(key)) return false;
    if (!legacyNames && key.size() == 32 && !hasArtifacts(key)) {
      Journal legacy(io, true);
      if (legacy.hasArtifacts(key)) return legacy.load(key, data, output);
    }
    Entry a, b;
    bool aa = metadata(key, 0, a), bb = metadata(key, 1, b);
    if (bb && (!aa || b.generation > a.generation)) {
      std::swap(a, b);
      std::swap(aa, bb);
    }
    for (auto candidate : {std::make_pair(aa, a), std::make_pair(bb, b)})
      if (candidate.first) {
        auto e = candidate.second;
        Entry checked;
        if (readRecord(key, e.slot, data, checked) &&
            checked.generation == e.generation &&
            checked.checksum == e.checksum) {
          if (output)
            *output = e;
          return true;
        }
      }
    data.clear();
    return false;
  }
  Status save(const std::string &key, const Bytes &data, Entry info,
              uint64_t reserve) {
    lastFailure = "none";
    freeBeforeSave = 0;
    if (!keyValid(key) || data.empty() || data.size() > MaxPayload)
      return InvalidData;
    if (!io.ready)
      return MediaLost;
    if (!io.writable)
      return storageFailure("not_writable");
    // Do not migrate or overwrite existing records just to change filenames.
    if (!legacyNames && key.size() == 32 && !hasArtifacts(key)) {
      Journal legacy(io, true);
      if (legacy.hasArtifacts(key)) {
        auto result = legacy.save(key, data, info, reserve);
        lastFailure = legacy.lastFailure;
        freeBeforeSave = legacy.freeBeforeSave;
        return result;
      }
    }
    if (io.exists(path(key, ".delete")) && !erase(key))
      return storageFailure("delete_cleanup");
    freeBeforeSave = io.freeBytes();
    if (freeBeforeSave < reserve + data.size() + 4096)
      return NoSpace;
    Entry old;
    Bytes previous;
    bool had = load(key, previous, &old);
    if (!had && (io.exists(path(key, ".ia")) || io.exists(path(key, ".ib")) ||
                 io.exists(path(key, ".a")) || io.exists(path(key, ".b"))))
      return storageFailure("existing_record_unreadable");
    info.key = key;
    info.slot = had ? 1 - old.slot : 0;
    info.generation = had ? old.generation + 1 : 1;
    if (!info.generation)
      return storageFailure("generation_overflow");
    info.size = data.size();
    info.checksum = crc(data.data(), data.size());
    const uint64_t mount = io.generation;
    if (!io.mkdir("/tc"))
      return storageFailure("mkdir");
    // Invalidate the inactive metadata BEFORE overwriting that slot's payload.
    auto metaPath = path(key, info.slot ? ".ib" : ".ia");
    if (io.exists(metaPath) && !io.remove(metaPath))
      return storageFailure("invalidate_metadata");
    auto dataPath = path(key, info.slot ? ".b" : ".a");
    JsonDocument m;
    m["key"] = key;
    m["generation"] = info.generation;
    m["size"] = info.size;
    m["crc"] = info.checksum;
    m["name"] = info.name;
    m["account"] = info.account;
    m["revision"] = info.revision;
    m["sequence"] = info.sequence;
    std::string body;
    serializeJson(m, body);
    JsonDocument envelope;
    envelope["body"] = body;
    envelope["crc"] =
        crc(reinterpret_cast<const uint8_t *>(body.data()), body.size());
    Bytes index = encode(envelope);
    if (index.size() > 2048)
      return InvalidData;
    Bytes record(16 + index.size() + data.size());
    memcpy(record.data(), "TCR2", 4);
    put32(record.data() + 4, index.size());
    put32(record.data() + 8, data.size());
    std::copy(index.begin(), index.end(), record.begin() + 16);
    std::copy(data.begin(), data.end(), record.begin() + 16 + index.size());
    put32(record.data() + 12, crc(record.data() + 16, record.size() - 16));
    if (!io.write(dataPath, record)) {
      io.remove(dataPath);
      return storageFailure("write_record");
    }
    Bytes check;
    if (!io.read(dataPath, check, MaxPayload + 2064) || check != record) {
      io.remove(dataPath);
      return storageFailure("verify_record");
    }
    if (!io.ready || io.generation != mount)
      return MediaLost;
    if (!io.write(metaPath, index)) {
      io.remove(metaPath);
      io.remove(dataPath);
      return storageFailure("write_metadata");
    }
    Entry published;
    if (!metadata(key, info.slot, published) ||
        published.generation != info.generation)
      return storageFailure("verify_metadata");
    return io.ready && io.generation == mount ? Stored : MediaLost;
  }
  bool erase(const std::string &key) {
    if (!keyValid(key) || key.size() != 32 || !io.ready || !io.writable)
      return false;
    if (!io.mkdir("/tc") || !io.write(path(key, ".delete"), Bytes{1}))
      return false;
    if (!legacyNames) {
      Journal legacy(io, true);
      // Keep the short tombstone until BOTH layouts are gone, so a failed
      // delete cannot expose an old record through the compatibility fallback.
      if (legacy.hasArtifacts(key) && !legacy.erase(key)) return false;
    }
    for (const char *suffix : {".ia", ".ib", ".a", ".b"}) {
      auto p = path(key, suffix);
      if (io.exists(p) && !io.remove(p))
        return false;
    }
    return io.remove(path(key, ".delete"));
  }
};
// Page storage is bounded; scanning yields to the caller after each metadata
// file.
class Catalog {
public:
  Journal &internal;
  Journal &sd;
  bool byName = false, scanning = false, backwards = false;
  std::vector<Entry> page;
  size_t pageSize = 5;
  uint64_t counts[2] = {}, bytes[2] = {}, appBytes[2] = {};
  Entry after;
  bool hasAfter = false;
  Catalog(Journal &a, Journal &b) : internal(a), sd(b) {}
  bool before(const Entry &a, const Entry &b) const {
    if (byName && a.name != b.name)
      return a.name < b.name;
    if (!byName && a.sequence != b.sequence)
      return a.sequence > b.sequence;
    return a.key < b.key;
  }
  void start(bool names, const Entry *cursor = nullptr, bool reverse = false) {
    byName = names;
    backwards = reverse;
    hasAfter = cursor;
    if (cursor)
      after = *cursor;
    page.clear();
    counts[0] = counts[1] = bytes[0] = bytes[1] = appBytes[0] = appBytes[1] = 0;
    media = 0;
    scanning = true;
    iterator = internal.io.ready ? internal.io.list("/tc") : nullptr;
  }
  bool tick() {
    if (!scanning)
      return false;
    for (int budget = 0; budget < 4; budget++) {
      if (!iterator) {
        if (media == 0) {
          media = 1;
          iterator = sd.io.ready ? sd.io.list("/tc") : nullptr;
          continue;
        }
        if (backwards && page.empty()) {
          start(byName);
          return false;
        }
        scanning = false;
        return true;
      }
      std::string filename = iterator->next();
      if (filename.empty()) {
        iterator.reset();
        continue;
      }
      auto slash = filename.find_last_of('/');
      if (slash != std::string::npos)
        filename = filename.substr(slash + 1);
      appBytes[media] += (media ? sd : internal).io.fileSize("/tc/" + filename);
      auto dot = filename.find('.');
      if (dot == std::string::npos) continue;
      const std::string key = cardKey(filename.substr(0, dot));
      if (key.empty()) continue;
      const auto suffix = filename.substr(dot);
      if ((dot == 32 && suffix == ".delete") || (dot == 26 && suffix == ".del")) {
        auto &journal = media ? sd : internal;
        if (journal.io.writable)
          journal.erase(key);
        continue;
      }
      auto &journal = media ? sd : internal;
      // If both layouts exist, the short layout is authoritative, even when
      // corrupt or tombstoned. Count each ID once, without resurrecting old data.
      if (dot == 32 && journal.hasArtifacts(key)) continue;
      if (suffix != ".a") {
        // Enumerate records, not sidecars, so missing indexes are recoverable.
        if (suffix != ".b")
          continue;
        if (journal.io.exists("/tc/" + filename.substr(0, dot) + ".a"))
          continue;
      }
      Entry entry, other;
      Journal &here = media ? sd : internal;
      Journal &there = media ? internal : sd;
      if (!here.latest(key, entry))
        continue;
      counts[media]++;
      bytes[media] += entry.size;
      entry.medium = media;
      if (there.io.ready && there.latest(key, other)) {
        if (other.revision > entry.revision ||
            (other.revision == entry.revision &&
             other.sequence > entry.sequence) ||
            (other.revision == entry.revision &&
             other.sequence == entry.sequence && media == 1))
          continue;
      }
      if (hasAfter &&
          !(backwards ? before(entry, after) : before(after, entry)))
        continue;
      auto position = std::lower_bound(
          page.begin(), page.end(), entry,
          [this](const Entry &a, const Entry &b) { return before(a, b); });
      page.insert(position, entry);
      if (page.size() > pageSize) {
        if (backwards)
          page.erase(page.begin());
        else
          page.pop_back();
      }
    }
    return false;
  }

private:
  int media = 0;
  std::unique_ptr<Cursor> iterator;
};
} // namespace tc
