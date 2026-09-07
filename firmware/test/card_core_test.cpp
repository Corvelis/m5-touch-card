#include "CardStore.h"
#include "CardTransfer.h"
#include "NfcTiming.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
using namespace tc;
class MemoryCursor : public Cursor {
  std::vector<std::string> names;
  size_t index = 0;

public:
  explicit MemoryCursor(const std::map<std::string, Bytes> &files) {
    for (const auto &f : files)
      names.push_back(f.first);
  }
  std::string next() override {
    return index < names.size() ? names[index++] : "";
  }
};
class MemoryIO : public IO {
public:
  std::map<std::string, Bytes> files;
  int failWrite = -1, writes = 0, failRemove = -1, removes = 0;
  uint64_t capacity = 10000000;
  size_t maxNameBytes = 255;
  MemoryIO() {
    ready = writable = true;
    generation = 1;
  }
  bool read(const std::string &p, Bytes &b, size_t max) override {
    auto it = files.find(p);
    if (!ready || it == files.end() || it->second.size() > max)
      return false;
    b = it->second;
    return true;
  }
  bool write(const std::string &p, const Bytes &b) override {
    if (p.size() - p.find_last_of('/') - 1 > maxNameBytes) return false;
    if (!ready || !writable)
      return false;
    if (writes++ == failWrite) {
      files[p] = Bytes(b.begin(), b.begin() + b.size() / 2);
      return false;
    }
    files[p] = b;
    return true;
  }
  bool remove(const std::string &p) override {
    if (!ready || !writable || removes++ == failRemove)
      return false;
    files.erase(p);
    return true;
  }
  bool exists(const std::string &p) override { return files.count(p); }
  uint64_t fileSize(const std::string &p) override {
    return files.count(p) ? files[p].size() : 0;
  }
  bool mkdir(const std::string &) override { return ready && writable; }
  uint64_t freeBytes() override {
    uint64_t used = 0;
    for (const auto &f : files)
      used += f.second.size();
    return capacity > used ? capacity - used : 0;
  }
  std::unique_ptr<Cursor> list(const std::string &) override {
    return std::make_unique<MemoryCursor>(files);
  }
};
Bytes begin(uint32_t id, uint8_t target, const Bytes &payload) {
  Bytes p(23);
  p[0] = 0x54;
  p[1] = 0x43;
  p[2] = 2;
  p[3] = 2;
  put32(p.data() + 4, id);
  p[8] = 1;
  p[9] = target;
  p[10] = 1;
  put32(p.data() + 15, payload.size());
  put32(p.data() + 19, crc(payload.data(), payload.size()));
  return p;
}
Bytes command(uint8_t cmd, uint32_t id) {
  Bytes p{0x54, 0x43, 2, cmd, 0, 0, 0, 0};
  put32(p.data() + 4, id);
  return p;
}
void protocolTests() {
  Receiver r;
  r.beginSession(true, 1);
  uint8_t reply[32];
  Bytes payload{'{', '}'};
  auto b = begin(12, 1, payload);
  assert(r.handle(b.data(), b.size(), reply, 1) == 13 && reply[4] == Accepted);
  auto chunk = command(3, 12);
  chunk.resize(15);
  put32(chunk.data() + 8, 0);
  chunk[12] = 2;
  chunk[13] = '{';
  chunk[14] = '}';
  r.handle(chunk.data(), chunk.size(), reply, 2);
  assert(reply[4] == Receiving && r.offset == 2);
  r.handle(chunk.data(), chunk.size(), reply, 3);
  assert(reply[4] == Receiving && r.offset == 2);
  chunk[13] = '!';
  r.handle(chunk.data(), chunk.size(), reply, 4);
  assert(reply[4] == Mismatch);
  chunk[13] = '{';
  auto commit = command(5, 12);
  commit.resize(12);
  put32(commit.data() + 8, crc(payload.data(), 2));
  r.handle(commit.data(), commit.size(), reply, 5);
  assert(reply[4] == Verifying && r.commitPending);
  r.commitPending = false;
  r.status = Stored;
  r.handle(b.data(), b.size(), reply, 6);
  assert(reply[4] == Stored);
  auto other = begin(13, 5, payload);
  r.handle(other.data(), other.size(), reply, 7);
  assert(reply[4] == WrongSession);
  r.beginSession(false, 2);
  r.handle(b.data(), b.size(), reply, 8);
  assert(reply[4] == WrongSession);
  r.handle(other.data(), other.size(), reply, 9);
  assert(reply[4] == Accepted);
  auto abort = command(6, 13);
  r.handle(abort.data(), abort.size(), reply, 10);
  assert(reply[4] == Ok);
  r.handle(other.data(), other.size(), reply, 11);
  assert(reply[4] == Accepted && r.status == Receiving);
  for (size_t n = 0; n < 23; n++) {
    r.handle(other.data(), n, reply, 12);
    if (n >= 4)
      assert(reply[4] == BadLength);
  }
  other[2] = 1;
  r.handle(other.data(), other.size(), reply, 13);
  assert(reply[4] == BadVersion);
  r.beginSession(true, 1);
  Bytes time(15);
  time[0] = 0x54;
  time[1] = 0x43;
  time[2] = 2;
  time[3] = 7;
  put32(time.data() + 4, 1800000000);
  put16(time.data() + 12, 540);
  r.handle(time.data(), time.size(), reply, 14);
  assert(reply[4] == Busy && r.clockPending);
  r.clockPending = false;
  r.status = Stored;
  r.handle(time.data(), time.size(), reply, 15);
  assert(reply[4] == Ok);
  // Random malformed commands exercise bounds under ASan/UBSan.
  uint32_t random = 17;
  for (int attempt = 0; attempt < 20000; attempt++) {
    Bytes fuzz(4 + (attempt % 260));
    for (auto &value : fuzz) {
      random = random * 1664525 + 1013904223;
      value = random >> 24;
    }
    fuzz[0] = 0x54;
    fuzz[1] = 0x43;
    fuzz[2] = 2;
    r.handle(fuzz.data(), fuzz.size(), reply, 16);
  }
}
void storageTests() {
  const std::string key(32, 'a');
  MemoryIO io;
  Journal j(io);
  Entry e;
  e.name = "Alice";
  e.revision = 1;
  e.sequence = 1;
  Bytes first{'o', 'l', 'd'}, second{'n', 'e', 'w'}, read;
  assert(j.save(key, first, e, 4096) == Stored);
  assert(j.load(key, read) && read == first);
  // Failure at either payload or metadata publication retains the prior
  // generation.
  auto initial = io.files;
  for (int failure = 0; failure < 2; failure++) {
    MemoryIO fresh; Journal freshJournal(fresh); fresh.failWrite = failure;
    assert(freshJournal.save(key, first, e, 4096) == StorageError);
    assert(std::string(freshJournal.lastFailure) ==
           (failure == 0 ? "write_record" : "write_metadata"));
    fresh.failWrite = -1;
    assert(freshJournal.save(key, first, e, 4096) == Stored);
  }
  for (int failure = 0; failure < 2; failure++) {
    io.files = initial;
    io.writes = 0;
    io.failWrite = failure;
    e.revision = 2;
    assert(j.save(key, second, e, 4096) == StorageError);
    assert(j.load(key, read) && read == first);
  }
  io.files = initial;
  io.failWrite = -1;
  assert(j.save(key, second, e, 4096) == Stored);
  assert(j.load(key, read) && read == second);
  io.files[j.path(key, ".b")][0] = '!';
  assert(j.load(key, read) && read == first);
  io.capacity = 1;
  assert(j.save(key, second, e, 4096) == NoSpace);
  io.capacity = 10000000;
  io.failRemove = io.removes;
  assert(!j.erase(key));
  assert(!j.load(key, read));
  assert(io.files.count(j.path(key, ".delete")));
  io.failRemove = -1;
  assert(j.erase(key));
  assert(!j.load(key, read));
  assert(io.files.empty());
  // Both sidecars can be rebuilt from complete checksummed records. A deleted
  // record remains masked even if an index is lost during interrupted removal.
  io.files = initial;
  io.files[j.path(key, ".ia")] = Bytes{'!'};
  assert(j.load(key, read) && read == first);
  io.files.erase(j.path(key, ".ia"));
  io.writable = false;
  assert(j.load(key, read) && read == first);
  io.writable = true;
  io.files[j.path(key, ".delete")] = Bytes{1};
  assert(!j.load(key, read));
  assert(j.erase(key));
  assert(!j.erase("own"));
  assert(!j.erase("../other"));
  assert(j.save("../other", first, e, 0) == InvalidData);
  io.writable = false;
  assert(j.save(key, first, e, 0) == StorageError);
  io.writable = true;
  io.ready = false;
  assert(j.save(key, first, e, 0) == MediaLost);
  MemoryIO a, b;
  Journal ja(a), jb(b);
  Catalog catalog(ja, jb);
  catalog.pageSize = 3;
  for (int i = 0; i < 10; i++) {
    char keyBuffer[33];
    snprintf(keyBuffer, sizeof(keyBuffer), "%032x", i + 1);
    Entry item;
    item.name = std::string(1, char('A' + i));
    item.sequence = i + 1;
    item.revision = 1;
    assert(ja.save(keyBuffer, first, item, 0) == Stored);
    if (i == 2) {
      item.revision = 2;
      assert(jb.save(keyBuffer, second, item, 0) == Stored);
    }
  }
  catalog.start(false);
  while (catalog.scanning)
    catalog.tick();
  assert(catalog.page.size() == 3 && catalog.page.front().name == "J");
  Entry cursor = catalog.page.back();
  catalog.start(false, &cursor);
  while (catalog.scanning)
    catalog.tick();
  assert(catalog.page.front().name == "G");
  cursor = catalog.page.front();
  catalog.start(false, &cursor, true);
  while (catalog.scanning)
    catalog.tick();
  assert(catalog.page.front().name == "J" && catalog.page.back().name == "H");
  catalog.start(true);
  while (catalog.scanning)
    catalog.tick();
  assert(catalog.page.front().name == "A" && catalog.page[2].medium == 1);
  assert(catalog.counts[0] == 10 && catalog.counts[1] == 1);
  assert(catalog.appBytes[0] > catalog.bytes[0]);
  for (auto it = a.files.begin(); it != a.files.end();) {
    if (it->first.substr(it->first.size() - 3) == ".ia")
      it = a.files.erase(it);
    else
      ++it;
  }
  catalog.start(true);
  while (catalog.scanning)
    catalog.tick();
  assert(catalog.counts[0] == 10 && catalog.page.front().name == "A");
  b.ready = false;
  catalog.start(true);
  while (catalog.scanning)
    catalog.tick();
  assert(catalog.page[2].medium == 0);
  assert(textValid("日本語😀", 4));
  assert(!textValid("日本語😀", 3));
  assert(!textValid("\xC0\xAF", 20));
  assert(!textValid("bad\nname", 80));
}
void sharedVectors(const char *filename) {
  std::ifstream file(filename);
  JsonDocument fixture;
  assert(file && !deserializeJson(fixture, file));
  auto bytes = [&](const char *key) {
    std::string hex = fixture[key].as<std::string>();
    Bytes b;
    for (size_t i = 0; i < hex.size(); i += 2)
      b.push_back(std::stoul(hex.substr(i, 2), nullptr, 16));
    return b;
  };
  auto payload = bytes("payload");
  auto b = bytes("begin");
  assert(begin(0x12345678, 1, payload) == b);
  Receiver receiver;
  receiver.beginSession(true, 1);
  uint8_t out[32] = {};
  auto hello = bytes("hello");
  auto length = receiver.handle(hello.data(), hello.size(), out, 1);
  auto expectedHello = bytes("hello_response");
  // Keep the old vector for compatibility tests in all existing phone clients.
  expectedHello[23] |= CompactCardCapability | FinishCapability;
  assert(Bytes(out, out + length) == expectedHello);
  receiver.handle(b.data(), b.size(), out, 2);
  assert(out[4] == Accepted);
  auto chunk = bytes("data");
  receiver.handle(chunk.data(), chunk.size(), out, 3);
  assert(receiver.offset == payload.size());
  auto commit = bytes("commit");
  receiver.handle(commit.data(), commit.size(), out, 4);
  assert(receiver.commitPending && out[4] == Verifying);
  assert(receiver.expectedCrc == crc(payload.data(), payload.size()));
  receiver.commitPending = false;
  receiver.status = Stored;
  auto status = command(4, 0x12345678);
  length = receiver.handle(status.data(), status.size(), out, 5);
  assert(Bytes(out, out + length) == bytes("stored_response"));
  JsonDocument card;
  assert(parse(payload, card) && profileValid(card["profile"]));
}
void filenameCompatibilityTests() {
  const std::string key = "0c4fde19d78cc4e1cf1e2deb5bb099fc";
  Entry e;
  e.name = "Compatibility";
  e.revision = e.sequence = 1;
  Bytes first{'o', 'l', 'd'}, second{'n', 'e', 'w'}, read;
  MemoryIO io, empty;
  Journal journal(io), legacy(io, true), other(empty);
  io.maxNameBytes = 32;
  // Reproduce the hardware failure before proving the new layout succeeds.
  assert(legacy.save(key, first, e, 0) == StorageError);
  assert(std::string(legacy.lastFailure) == "write_record");
  assert(journal.save(key, first, e, 0) == Stored);
  assert(journal.load(key, read) && read == first);
  e.revision = 2;
  assert(journal.save(key, second, e, 0) == Stored);
  for (const auto &f : io.files)
    assert(f.first.size() - f.first.find_last_of('/') - 1 <= 32);
  Catalog catalog(journal, other);
  catalog.start(true);
  while (catalog.scanning) catalog.tick();
  assert(catalog.page.size() == 1 && catalog.page[0].key == key);
  assert(catalog.counts[0] == 1);
  assert(journal.erase(key) && io.files.empty());
  // Interrupted deletes remain masked and can be resumed on a 32-byte FS.
  assert(journal.save(key, first, e, 0) == Stored);
  io.failRemove = io.removes;
  assert(!journal.erase(key));
  assert(!journal.load(key, read));
  io.failRemove = -1;
  assert(journal.erase(key) && io.files.empty());

  // Existing SD / wider-name volumes retain their old layout and generations.
  io.maxNameBytes = 255;
  assert(legacy.save(key, first, e, 0) == Stored);
  auto originals = io.files;
  assert(journal.load(key, read) && read == first);
  assert(io.files == originals); // no migration on read
  assert(journal.save(key, second, e, 0) == Stored);
  assert(!io.files.count(journal.path(key, ".a")));
  assert(journal.load(key, read) && read == second);
  catalog.start(false);
  while (catalog.scanning) catalog.tick();
  assert(catalog.page.size() == 1 && catalog.counts[0] == 1);
  // Both names for the same ID must not produce duplicates or resurrect data.
  auto oldLayout = io.files;
  io.files.clear();
  assert(journal.save(key, first, e, 0) == Stored);
  io.files.insert(oldLayout.begin(), oldLayout.end());
  catalog.start(false);
  while (catalog.scanning) catalog.tick();
  assert(catalog.page.size() == 1 && catalog.counts[0] == 1);
  assert(journal.load(key, read) && read == first);
  io.failRemove = io.removes;
  assert(!journal.erase(key));
  assert(!journal.load(key, read));
  io.failRemove = -1;
  assert(journal.erase(key) && io.files.empty());

  assert(cardStem(std::string(32, '0')) == std::string(26, 'a'));
  assert(cardStem(std::string(32, 'f')) == std::string(25, '7') + "4");
  assert(cardKey(std::string(25, 'a') + "b").empty());
  assert(cardKey(std::string(26, '!')).empty());
  uint32_t random = 123;
  for (int i = 0; i < 10000; ++i) {
    std::string hex;
    for (int n = 0; n < 32; ++n) {
      random = random * 1664525 + 1013904223;
      hex += "0123456789abcdef"[random >> 28];
    }
    assert(cardKey(cardStem(hex)) == hex);
    assert(journal.path(hex, ".delete").size() - 4 == 30);
  }
}
int main(int argc, char **argv) {
  assert(argc == 2);
  sharedVectors(argv[1]);
  protocolTests();
  storageTests();
  filenameCompatibilityTests();
  assert(cardResponseTimeout(2) == 80 && cardResponseTimeout(4) == 150);
  assert(cardRetryChunk(240, 2) == 240);
  assert(cardRetryChunk(240, 5) == 240);
  assert(cardRetryChunk(240, 6) == 128);
  assert(cardRetryChunk(128, 11) == 128);
  assert(cardRetryChunk(128, 12) == 64);
  assert(cardRetryChunk(64, 20) == 64);
  assert(waitForCardSave(2999) && !waitForCardSave(3000));
  std::cout << "card wire, journal, deletion and catalog: PASS\n";
}
