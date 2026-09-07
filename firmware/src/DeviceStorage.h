#pragma once
#include <UiLanguage.h>
#include "BoardStorage.h"
#include <CardStore.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <mbedtls/base64.h>
#include <sys/stat.h>

class FsCursor final : public tc::Cursor {
  File directory;

public:
  explicit FsCursor(fs::FS &fs, const char *path) : directory(fs.open(path)) {}
  std::string next() override {
    if (!directory)
      return {};
    auto file = directory.openNextFile();
    if (!file)
      return {};
    std::string name = file.name();
    file.close();
    return name;
  }
};
class ArduinoIO final : public tc::IO {
public:
  struct Performance {
    uint32_t readUs = 0, writeUs = 0, statUs = 0, freeUs = 0;
    uint32_t reads = 0, writes = 0, stats = 0, frees = 0;
  };
  mutable Performance performance;
  struct Measure {
    uint32_t &total;
    uint32_t started;
    Measure(uint32_t &us, uint32_t &count) : total(us), started(micros()) { ++count; }
    ~Measure() { total += micros() - started; }
  };
  void logPerformance(const char *medium) const {
    Serial.printf("[tc.store.cost] medium=%s read_us=%lu/%lu write_us=%lu/%lu stat_us=%lu/%lu free_us=%lu/%lu\n",
        medium, (unsigned long)performance.readUs, (unsigned long)performance.reads,
        (unsigned long)performance.writeUs, (unsigned long)performance.writes,
        (unsigned long)performance.statUs, (unsigned long)performance.stats,
        (unsigned long)performance.freeUs, (unsigned long)performance.frees);
  }
  fs::FS *fs = nullptr;
  const char *mountpoint = nullptr;
  bool info(const std::string &path, struct stat &st) const {
    Measure measure(performance.statUs, performance.stats);
    if (!ready || !fs || !mountpoint || path.empty() || path[0] != '/') return false;
    return ::stat((std::string(mountpoint) + path).c_str(), &st) == 0;
  }
  std::function<uint64_t()> free;
  tc::Bytes volumeIdentity;
  bool sameVolume() {
    if (!ready || !fs)
      return false;
    if (volumeIdentity.empty())
      return true;
    tc::Bytes actual;
    if (read("/tc/.volume", actual, 16) && actual == volumeIdentity)
      return true;
    ready = writable = false;
    generation++;
    return false;
  }
  bool read(const std::string &path, tc::Bytes &data, size_t limit) override {
    Measure measure(performance.readUs, performance.reads);
    if (!ready || !fs)
      return false;
    // Missing journal generations are normal. Avoid FS::open's error log and
    // directory probing for every absent sidecar / compatibility filename.
    struct stat st{};
    if (!info(path, st) || !S_ISREG(st.st_mode) || st.st_size <= 0 ||
        uint64_t(st.st_size) > limit) return false;
    auto f = fs->open(path.c_str(), FILE_READ);
    if (!f)
      return false;
    size_t size = f.size();
    if (!size || size > limit) {
      f.close();
      return false;
    }
    data.resize(size);
    size_t read = 0;
    while (read < size) {
      size_t n =
          f.read(data.data() + read, std::min(size - read, size_t(4096)));
      if (!n)
        break;
      read += n;
      yield();
    }
    f.close();
    return read == size;
  }
  bool write(const std::string &path, const tc::Bytes &data) override {
    Measure measure(performance.writeUs, performance.writes);
    if (!ready || !writable || !fs || !sameVolume())
      return false;
    auto f = fs->open(path.c_str(), FILE_WRITE);
    if (!f)
      return false;
    size_t written = 0;
    while (written < data.size()) {
      size_t n = f.write(data.data() + written,
                         std::min(data.size() - written, size_t(4096)));
      if (!n)
        break;
      written += n;
      yield();
    }
    f.flush();
    f.close();
    return written == data.size();
  }
  bool remove(const std::string &p) override {
    return ready && writable && sameVolume() && fs->remove(p.c_str());
  }
  bool exists(const std::string &p) override {
    struct stat st{};
    return info(p, st);
  }
  uint64_t fileSize(const std::string &p) override {
    struct stat st{};
    return info(p, st) && S_ISREG(st.st_mode) ? st.st_size : 0;
  }
  bool mkdir(const std::string &p) override {
    return ready && fs && (exists(p) || fs->mkdir(p.c_str()));
  }
  uint64_t freeBytes() override {
    Measure measure(performance.freeUs, performance.frees);
    return ready && free ? free() : 0;
  }
  std::unique_ptr<tc::Cursor> list(const std::string &p) override {
    return ready ? std::make_unique<FsCursor>(*fs, p.c_str()) : nullptr;
  }
};
class DeviceStorage {
public:
  BoardStorage board;
  ArduinoIO internal, sd;
  tc::Journal own{internal}, external{sd};
  tc::Catalog catalog{own, external};
  bool internalOnly = false;
  bool sdEjected = false, observedInsertion = false;
  uint32_t lastProbe = 0;
  uint64_t receiveSequence = 0;
  void begin() {
    board.begin();
    bindInternal();
    bindSd();
    observedInsertion = board.sd.state != touchcard::MediaState::Absent;
    catalog.start(false);
  }
  void bindInternal() {
    internal.fs = &LittleFS;
    internal.mountpoint = "/littlefs";
    internal.ready = board.internalMounted;
    internal.writable = internal.ready;
    internal.generation++;
    internal.free = [this]() {
      board.refreshInternal();
      return board.internal.free();
    };
  }
  void bindSd() {
    sd.volumeIdentity.clear();
#if TOUCH_CARD_PAPER_MONO
    sd.fs = &SD_MMC;
    sd.mountpoint = "/sdcard";
    sd.free = []() {
      uint64_t a = SD_MMC.totalBytes(), b = SD_MMC.usedBytes();
      return a > b ? a - b : 0;
    };
#else
    sd.fs = &SD;
    sd.mountpoint = "/sd";
    sd.free = []() {
      uint64_t a = SD.totalBytes(), b = SD.usedBytes();
      return a > b ? a - b : 0;
    };
#endif
    sd.generation++;
    sd.ready = board.sdMounted && board.sd.valid();
    sd.writable = sd.ready;
    if (sd.ready) {
      tc::Bytes probe{0x54, 0x43};
      const std::string path = "/tc/.write-probe";
      tc::Bytes check;
      sd.writable = sd.mkdir("/tc") && sd.write(path, probe) &&
                    sd.read(path, check, 16) && check == probe;
      if (sd.writable && !sd.remove(path))
        sd.writable = false;
      tc::Bytes identity;
      if (!sd.read("/tc/.volume", identity, 16) || identity.size() != 16) {
        // A damaged identity is never overwritten automatically.
        if (sd.exists("/tc/.volume"))
          sd.writable = false;
        else if (sd.writable) {
          identity.resize(16);
          for (size_t i = 0; i < 16; i += 4)
            tc::put32(identity.data() + i, esp_random());
          if (!sd.write("/tc/.volume", identity) ||
              !sd.read("/tc/.volume", check, 16) || check != identity)
            sd.writable = false;
        }
      }
      if (identity.size() == 16)
        sd.volumeIdentity = std::move(identity);
      if (!sd.writable)
        board.sd.state = touchcard::MediaState::ReadOnly;
    }
  }
  void eject() {
    sdEjected = true;
    sd.ready = false;
    sd.writable = false;
    sd.generation++;
    board.eject();
    catalog.start(catalog.byName);
  }
  void mount() {
    sdEjected = false;
    board.mountSd();
    bindSd();
    catalog.start(catalog.byName);
  }
  void tick(bool transferActive) {
    if (!transferActive)
      catalog.tick();
    if (millis() - lastProbe < 1000)
      return;
    lastProbe = millis();
#if TOUCH_CARD_PAPER_MONO
    bool absent = false;
    if (M5.getIOExpander(0).getInputLevel(m5::M5IOE1_Class::gpio1, &absent)) {
      if (absent && sd.ready) {
        sd.ready = sd.writable = false;
        sd.generation++;
        board.eject();
        board.sd.state = touchcard::MediaState::Absent;
        catalog.start(catalog.byName);
      }
      if (absent) {
        sdEjected = false;
        observedInsertion = false;
      }
      if (!absent && !observedInsertion && !sdEjected && !transferActive) {
        observedInsertion = true;
        mount();
      }
    }
#else
    if (sd.ready) {
      auto root = sd.fs->open("/");
      if (!root) {
        sd.ready = sd.writable = false;
        sd.generation++;
        board.sdMounted = false;
        board.sd.state = touchcard::MediaState::Error;
        catalog.start(catalog.byName);
      } else
        root.close();
    }
    // CoreS3 has no independent insertion switch: bounded mount retry, never
    // format.
    static uint32_t lastMount = 0;
    if (!sd.ready && !sdEjected && !transferActive &&
        millis() - lastMount > 5000) {
      lastMount = millis();
      mount();
    }
#endif
  }
  String deleteFailure;
  bool erase(const std::string &id, uint8_t mask, uint64_t internalGeneration,
             uint64_t sdGeneration) {
    deleteFailure = "";
    if (!mask) {
      deleteFailure = tc::tr("保存先が見つかりません", "Storage unavailable");
      return false;
    }
    bool ok = true;
    for (int medium = 0; medium < 2; medium++) {
      if (!(mask & (1 << medium)))
        continue;
      auto &io = medium ? sd : internal;
      auto &journal = medium ? external : own;
      uint64_t expected = medium ? sdGeneration : internalGeneration;
      if (!io.sameVolume() || io.generation != expected || !journal.erase(id)) {
        ok = false;
        deleteFailure += medium ? " SD" : tc::tr(" 本体", " Internal");
      }
    }
    catalog.start(catalog.byName);
    return ok;
  }
  bool loadCard(const std::string &id, tc::Bytes &bytes,
                tc::Entry *result = nullptr) {
    tc::Entry a, b;
    tc::Bytes aa, bb;
    bool ia = internal.ready && own.load(id, aa, &a),
         ib = sd.ready && external.load(id, bb, &b);
    if (!ia && !ib)
      return false;
    bool chooseSd =
        ib && (!ia || b.revision > a.revision ||
               (b.revision == a.revision && b.sequence > a.sequence));
    bytes = chooseSd ? std::move(bb) : std::move(aa);
    if (result) {
      *result = chooseSd ? b : a;
      result->medium = chooseSd ? 1 : 0;
    }
    return true;
  }
};

inline bool decodeImage(JsonVariantConst image, tc::Bytes &bytes,
                        int maxWidth = 800, int maxHeight = 800) {
  if (!image.is<JsonObjectConst>() || !image["jpeg"].is<const char *>() ||
      !image["width"].is<int>() || !image["height"].is<int>())
    return false;
  const char *encoded = image["jpeg"];
  size_t length = strlen(encoded);
  if (length > 349528)
    return false;
  size_t required = 0;
  mbedtls_base64_decode(nullptr, 0, &required,
                        reinterpret_cast<const uint8_t *>(encoded), length);
  if (!required || required > 262144)
    return false;
  bytes.resize(required);
  if (mbedtls_base64_decode(bytes.data(), bytes.size(), &required,
                            reinterpret_cast<const uint8_t *>(encoded), length))
    return false;
  bytes.resize(required);
  if (required < 4 || bytes[0] != 255 || bytes[1] != 216 ||
      bytes[required - 2] != 255 || bytes[required - 1] != 217)
    return false;
  size_t pos = 2;
  while (pos + 4 < required) {
    if (bytes[pos++] != 255)
      continue;
    while (pos < required && bytes[pos] == 255)
      pos++;
    if (pos >= required)
      break;
    uint8_t marker = bytes[pos++];
    if (marker == 0xda || marker == 0xd9)
      break;
    if (marker == 1 || (marker >= 0xd0 && marker <= 0xd7))
      continue;
    if (pos + 2 > required)
      return false;
    size_t size = (bytes[pos] << 8) | bytes[pos + 1];
    if (size < 2 || size > required - pos)
      return false;
    if (marker == 0xc2)
      return false;
    if (marker == 0xc0) {
      if (size < 17 || bytes[pos + 2] != 8 || bytes[pos + 7] != 3)
        return false;
      int height = (bytes[pos + 3] << 8) | bytes[pos + 4],
          width = (bytes[pos + 5] << 8) | bytes[pos + 6];
      return width > 0 && height > 0 && width <= maxWidth &&
             height <= maxHeight && width == image["width"].as<int>() &&
             height == image["height"].as<int>();
    }
    pos += size;
  }
  return false;
}
class CardService {
public:
  DeviceStorage &storage;
  String lastReceivedId;
  explicit CardService(DeviceStorage &s) : storage(s) {}
  tc::Status apply(uint8_t target, const tc::Bytes &data,
                   ArduinoIO *destination, uint64_t generation) {
    if (!destination || !destination->sameVolume() ||
        destination->generation != generation)
      return tc::MediaLost;
    JsonDocument incoming;
    if (!tc::parse(data, incoming) || incoming["target"] != target)
      return tc::InvalidData;
    JsonDocument output;
    std::string key;
    tc::Entry entry;
    uint64_t reserve =
        destination == &storage.sd ? 65536 : BoardStorage::kInternalReserve;
    if (target == 1 || target == 2 || target == 5) {
      tc::Bytes old;
      JsonDocument previous;
      if (target != 5 && storage.own.load("own", old))
        tc::parse(old, previous);
      if (target == 2) {
        if (!tc::profileValid(previous["profile"]) ||
            incoming["profileId"] != previous["profile"]["id"] ||
            !incoming["revision"].is<uint32_t>())
          return tc::Conflict;
        output.set(previous);
        output["profile"]["revision"] = incoming["revision"];
        output["avatar"] = incoming["avatar"];
      } else {
        if (!tc::profileValid(incoming["profile"]))
          return tc::InvalidData;
        output["profile"] = incoming["profile"];
        if (incoming["avatar"].isUnbound() && target == 1 &&
            incoming["profile"]["id"] == previous["profile"]["id"])
          output["avatar"] = previous["avatar"];
        else
          output["avatar"] = incoming["avatar"];
      }
      if (!output["avatar"].isNull()) {
        tc::Bytes image;
        if (!decodeImage(output["avatar"], image, 256, 256))
          return tc::InvalidData;
      }
      if (target != 5) {
        bool preserve = target == 1 && incoming["avatar"].isUnbound() &&
                        incoming["profile"]["id"] == previous["profile"]["id"];
        auto exchange = preserve ? previous["exchangeAvatar"] : incoming["exchangeAvatar"];
        output.remove("exchangeAvatar");
        if (!output["avatar"].isNull() && !exchange.isNull()) {
          tc::Bytes image;
          if (!decodeImage(exchange, image, 256, 256)) return tc::InvalidData;
          output["exchangeAvatar"] = exchange;
        }
      }
      entry.key = output["profile"]["id"].as<std::string>();
      entry.name = output["profile"]["name"].as<std::string>();
      entry.account = output["profile"]["account"].as<std::string>();
      entry.revision = output["profile"]["revision"];
      key = target == 5 ? entry.key : "own";
      tc::Entry oldEntry;
      tc::Bytes existing;
      JsonDocument oldCard;
      bool had = target == 5 ? storage.loadCard(key, existing, &oldEntry)
                             : storage.own.load(key, existing, &oldEntry);
      if (had && tc::parse(existing, oldCard) &&
          oldCard["profile"]["id"] == output["profile"]["id"]) {
        if (entry.revision < oldEntry.revision)
          return tc::Conflict;
        if (entry.revision == oldEntry.revision) {
          if (tc::encode(output) != tc::encode(oldCard))
            return tc::Conflict;
          if (target != 5)
            return tc::Stored;
        }
      }
      // NVS sequence is monotonic even before a clock is set; do not index all
      // SD cards in NVS.
      Preferences p;
      if (!p.begin("tc_sequence", false)) {
        Serial.printf("[tc.store] stage=sequence_open status=16\n");
        return tc::StorageError;
      }
      uint64_t seq = p.getULong64("next", 0) + 1;
      bool saved = seq && p.putULong64("next", seq) == 8;
      p.end();
      if (!saved) {
        Serial.printf("[tc.store] stage=sequence_write status=16 overflow=%u\n", !seq);
        return tc::StorageError;
      }
      entry.sequence = seq;
    } else if (target == 3 || target == 4) {
      tc::Bytes image;
      if (!decodeImage(incoming["image"], image))
        return tc::InvalidData;
#if TOUCH_CARD_PAPER_MONO
      int w = target == 3 ? 386 : 480, h = target == 3 ? 386 : 800;
#else
      int w = target == 3 ? 144 : 320, h = target == 3 ? 144 : 240;
#endif
      if (incoming["image"]["width"] != w || incoming["image"]["height"] != h)
        return tc::Unsupported;
      output["image"] = incoming["image"];
      key = target == 3 ? "dashboard" : "fullscreen";
    } else
      return tc::Unsupported;
    tc::Journal &journal =
        destination == &storage.sd ? storage.external : storage.own;
    auto result = journal.save(key, tc::encode(output), entry, reserve);
    Serial.printf("[tc.store] target=%u medium=%s status=%u stage=%s ready=%u writable=%u free_before=%llu\n",
                  target, destination == &storage.sd ? "sd" : "internal",
                  unsigned(result), journal.lastFailure, destination->ready,
                  destination->writable, (unsigned long long)journal.freeBeforeSave);
    if (result == tc::Stored) {
      storage.catalog.start(storage.catalog.byName);
      if (target == 5)
        lastReceivedId = key.c_str();
    }
    return result;
  }
};
