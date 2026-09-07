#include "CardWire.h"
#include "NfcAFrame.h"
#include <cassert>
#include <iostream>
using namespace tc;

int main() {
  // Known NFC-A READ frame: 30 04 26 EE. Guard CRC byte order and seed.
  const uint8_t readCommand[] = {0x30, 0x04, 0x26, 0xEE};
  uint16_t readSize = sizeof(readCommand);
  assert(nfcACrc(readCommand, 2) == 0xEE26);
  assert(nfcAPayloadSize(readCommand, readSize) && readSize == 2);
  readSize = 4;
  assert(!nfcAPayloadSize(nullptr, readSize) && readSize == 4);
  const uint64_t token = 0xf123456789abcdefULL;
  Receiver r;
  uint8_t response[32] = {};
  auto handle = [&](const std::vector<uint8_t> &p) {
    auto size = r.handle(p.data(), p.size(), response, 100);
    assert(size >= 13 && size <= sizeof(response));
    return Status(response[4]);
  };
  auto pair = [&](uint64_t t, uint8_t leg) {
    std::vector<uint8_t> p{0x54, 0x43, 2, 9}; p.resize(13);
    put32(p.data() + 4, uint32_t(t)); put32(p.data() + 8, uint32_t(t >> 32)); p[12] = leg;
    return p;
  };
  std::vector<uint8_t> begin{0x54, 0x43, 2, 2}; begin.resize(23);
  put32(begin.data() + 4, 42); begin[8] = 1; begin[9] = 5; begin[10] = 2;
  put32(begin.data() + 15, 100); put32(begin.data() + 19, 123);
  r.beginSession(false, 1);
  assert(handle({0x54,0x43,2,1}) == Ok && response[23] == 31);
  assert(handle(pair(token, 1)) == WrongSession);
  assert(handle(begin) == Accepted); // Ordinary one-way remains compatible.
  r.beginSession(false, 1, 1);
  assert(handle({0x54,0x43,2,1}) == Ok && (response[23] & MutualCapability) && response[26] == 1);
  assert(handle(begin) == WrongSession && r.id == 0); // Must agree before DATA.
  assert(handle(pair(0, 1)) == InvalidData);
  assert(handle(pair(token, 2)) == WrongSession);
  for (size_t size : {size_t(4), size_t(8), size_t(12), size_t(14)}) {
    auto p = pair(token, 1); p.resize(size);
    assert(handle(p) == BadLength);
  }
  assert(handle(pair(token, 1)) == Ok && r.mutualToken == token && r.mutualPaired);
  assert(le32(response + 13) == uint32_t(token) && le32(response + 17) == uint32_t(token >> 32));
  assert(handle(pair(token, 1)) == Ok); // Lost PAIR response retry.
  assert(handle(pair(token + 1, 1)) == Conflict && r.mutualToken == token);
  assert(handle(begin) == Accepted);
  // Reverse direction must carry the same session, not a nearby third device.
  r.beginSession(false, 2, 2, token);
  assert(!r.mutualPaired && handle(begin) == WrongSession);
  assert(handle(pair(token + 1, 2)) == Conflict && !r.mutualPaired);
  assert(handle(pair(token, 1)) == WrongSession);
  assert(handle(pair(token, 2)) == Ok && handle(begin) == Accepted);
  r.cancel();
  assert(!r.active && !r.mutualPaired && !r.mutualToken && !r.mutualLeg);
  r.beginSession(true, 1, 1); // Phone never advertises mutual mode.
  assert(handle({0x54,0x43,2,1}) == Ok && response[23] == 31);
  assert(handle(pair(token, 1)) == WrongSession);
  r.beginSession(false, 1, 2); // Missing reverse-session token fails closed.
  assert(handle(pair(token, 2)) == Conflict);

  // Real reader API returns the two CRC bytes that Receiver::handle omits.
  // Cover both roles and board types, including the original 24 != 22 failure.
  for (uint8_t board : {1, 2}) for (uint8_t leg : {1, 2}) {
    Receiver peer;
    peer.beginSession(false, board, leg, leg == 2 ? token : 0);
    const auto request = pair(token, leg);
    std::vector<uint8_t> frame(32);
    auto payloadSize = peer.handle(request.data(), request.size(), frame.data(), 100);
    assert(payloadSize == 22 && frame[4] == Ok);
    frame.resize(payloadSize + 2);
    put16(frame.data() + payloadSize, nfcACrc(frame.data(), payloadSize));
    uint16_t size = frame.size();
    assert(size == 24); // Previously compared directly against 22 and failed.
    assert(nfcAPayloadSize(frame.data(), size) && size == 22);
    assert(frame[21] == leg &&
           (uint64_t(le32(frame.data() + 13)) | (uint64_t(le32(frame.data() + 17)) << 32)) == token);
    for (size_t i = 0; i < frame.size(); ++i) {
      auto corrupt = frame; corrupt[i] ^= 1;
      size = corrupt.size();
      assert(!nfcAPayloadSize(corrupt.data(), size) && size == corrupt.size());
    }
    for (uint16_t cut = 0; cut < frame.size(); ++cut) {
      size = cut;
      assert(!nfcAPayloadSize(frame.data(), size) && size == cut);
    }
    frame.push_back(0); size = frame.size();
    // CRC is not a length delimiter: a zero extension can retain its residue.
    // The PAIR payload's strict length check must still reject extra bytes.
    assert(!nfcAPayloadSize(frame.data(), size) || size != 22);

    // Normal HELLO and rejection responses still decode at their exact lengths.
    for (const auto &command : {std::vector<uint8_t>{0x54,0x43,2,1}, pair(token + 1, leg)}) {
      frame.resize(32);
      payloadSize = peer.handle(command.data(), command.size(), frame.data(), 200);
      assert(payloadSize == (command[3] == 1 ? 27U : 13U));
      frame.resize(payloadSize + 2);
      put16(frame.data() + payloadSize, nfcACrc(frame.data(), payloadSize));
      size = frame.size();
      assert(nfcAPayloadSize(frame.data(), size) && size == payloadSize);
      assert(frame[4] == (command[3] == 1 ? Ok : Conflict));
    }
  }

  for (bool firstSender : {true, false}) {
    MutualExchange flow; flow.start(firstSender);
    assert(flow.leg == 1 && flow.sender() == firstSender && !flow.readyForNext(9999));
    flow.token = token;
    flow.completeLeg(UINT32_MAX - 100);
    assert(flow.active && flow.leg == 2 && flow.switching);
    assert(flow.sender() != firstSender);
    assert(flow.sent == firstSender && flow.received != firstSender);
    flow.completeLeg(0); // Cannot count completion twice while switching.
    assert(flow.active && !(flow.sent && flow.received));
    if (!firstSender) assert(!flow.readyForNext(100));
    assert(flow.readyForNext(1200)); // Unsigned millis rollover.
    flow.switching = false;
    assert(!flow.readyForNext(1300));
    flow.completeLeg(1500);
    assert(!flow.active && flow.sent && flow.received && flow.token == token);
    flow.reset(); assert(!flow.active && !flow.switching && !flow.token && !flow.leg);
  }
  std::cout << "Mutual negotiation, role reversal, retry and cancellation: PASS\n";
}
