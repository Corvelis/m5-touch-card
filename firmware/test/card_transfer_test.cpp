#include "CardTransfer.h"
#include <cassert>
#include <iostream>
using namespace tc;
Bytes command(uint8_t op, uint32_t id) {
  Bytes p{0x54, 0x43, 2, op, 0, 0, 0, 0};
  put32(p.data() + 4, id);
  return p;
}
int main() {
  JsonDocument own;
  assert(!deserializeJson(own, R"({"profile":{"id":"00112233445566778899aabbccddeeff","revision":2,"name":"日本語","account":"@name","email":"","url":"https://example.com","comment":"こんにちは"},"avatar":null})"));
  JsonDocument card, decoded;
  exchangeCard(own, card);
  Bytes wire, json;
  assert(packCard(card, wire) && unpackCard(wire, json) && json == encode(card));
  Bytes large(30000), small(8192);
  for (size_t i = 0; i < large.size(); ++i) large[i] = i * 13;
  for (size_t i = 0; i < small.size(); ++i) small[i] = i * 17;
  own["avatar"]["jpeg"] = base64Encode(large);
  own["avatar"]["width"] = 256; own["avatar"]["height"] = 256;
  own["exchangeAvatar"]["jpeg"] = base64Encode(small);
  own["exchangeAvatar"]["width"] = 160; own["exchangeAvatar"]["height"] = 160;
  auto original = encode(own);
  exchangeCard(own, card);
  assert(encode(own) == original); // Selecting a transfer derivative is read-only.
  assert(card["avatar"]["jpeg"] == own["exchangeAvatar"]["jpeg"]);
  assert(card["avatar"]["width"] == 160 && card["avatar"]["height"] == 160);
  assert(own["avatar"]["width"] == 256 && own["avatar"]["height"] == 256);
  assert(packCard(card, wire) && unpackCard(wire, json) && json == encode(card));
  assert(wire.size() < 8704 && json.size() > wire.size());
  assert(!card["exchangeAvatar"].is<JsonObject>());
  // Padding, noncanonical base64, bounds, and trailing bytes.
  for (size_t n = 1; n < 256; ++n) {
    Bytes source(large.begin(), large.begin() + n), restored;
    assert(base64Decode(base64Encode(source), restored) && source == restored);
  }
  Bytes restored;
  for (auto bad : {"", "A", "====", "AA=A", "AB==", "AAB=", "AA==AAAA", "!!!!"})
    assert(!base64Decode(bad, restored));
  for (size_t n = 0; n < wire.size(); n += 31)
    assert(!unpackCard(Bytes(wire.begin(), wire.begin() + n), restored));
  auto bad = wire; bad.push_back(0); assert(!unpackCard(bad, restored));
  bad = wire; put32(bad.data() + 4, 0xffffffff); assert(!unpackCard(bad, restored));
  bad = wire; put32(bad.data() + 8, 262145); assert(!unpackCard(bad, restored));
  bad = wire; bad[0] = '!'; assert(!unpackCard(bad, restored));

  Receiver r; r.beginSession(false, 1);
  uint8_t reply[32] = {};
  Bytes begin(23); begin[0] = 0x54; begin[1] = 0x43; begin[2] = 2; begin[3] = 2;
  put32(begin.data() + 4, 17); begin[8] = 1; begin[9] = 5; begin[10] = 2;
  put32(begin.data() + 15, wire.size());
  put32(begin.data() + 19, crc(wire.data(), wire.size()));
  auto handle = [&](const Bytes &b) { r.handle(b.data(), b.size(), reply, 1); return Status(reply[4]); };
  assert(handle(begin) == Accepted && r.encoding == 2);
  auto finish = command(8, 17);
  assert(handle(finish) == Receiving && !r.finishRequested);
  auto conflict = begin; conflict[10] = 1;
  assert(handle(conflict) == Conflict);
  // Full-sized DATA, then 128/64 byte fallback at the SAME transaction/offset.
  size_t index = 0;
  while (r.offset < wire.size()) {
    size_t count = std::min(wire.size() - r.offset, index++ < 2 ? size_t(240) : index < 4 ? size_t(128) : size_t(64));
    auto chunk = command(3, 17); chunk.resize(13 + count);
    put32(chunk.data() + 8, r.offset); chunk[12] = count;
    memcpy(chunk.data() + 13, wire.data() + r.offset, count);
    assert(handle(chunk) == Receiving);
    auto offset = r.offset;
    assert(handle(chunk) == Receiving && r.offset == offset); // Lost response retry.
  }
  auto commit = command(5, 17); commit.resize(12);
  put32(commit.data() + 8, r.expectedCrc);
  assert(handle(commit) == Verifying && r.commitPending);
  assert(handle(finish) == Verifying && !r.finishRequested);
  assert(crc(r.buffer.data(), r.size) == r.expectedCrc);
  assert(unpackCard(Bytes(r.buffer.begin(), r.buffer.begin() + r.size), restored));
  assert(restored == json);
  r.commitPending = false; r.status = StorageError;
  assert(handle(finish) == StorageError && !r.finishRequested);
  r.status = Stored;
  assert(handle(finish) == Stored && r.finishRequested);
  assert(handle(finish) == Stored); // Lost FINISH response is idempotent.
  assert(handle(command(8, 18)) == NotFound);
  auto badFinish = finish; badFinish.push_back(0);
  assert(handle(badFinish) == BadLength);
  put32(begin.data() + 4, 18);
  assert(handle(begin) == Accepted && !r.finishRequested);
  r.beginSession(true, 1); begin[9] = 1;
  assert(handle(begin) == Unsupported); // Compact cannot mutate own data.
  begin[10] = 1; assert(handle(begin) == Accepted);
  r.status = Stored;
  assert(handle(command(8, 18)) == WrongSession);
  assert(!exchangeUiReady(false, false, 3999, 0));
  assert(exchangeUiReady(false, false, 4000, 0));
  assert(!exchangeUiReady(false, true, 100, 19));
  assert(exchangeUiReady(false, true, 100, 20));
  assert(exchangeUiReady(true, false, 0, 0));
  std::cout << "compact cards, chunk fallback, durability and finish: PASS\n";
}
