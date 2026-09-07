#pragma once
#include <UiLanguage.h>
#include "DeviceStorage.h"
#include "RtcClock.h"
#include <CardTransfer.h>
#include <NfcTiming.h>
#include <NfcAFrame.h>
#include <TransferProgress.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedNFC.h>
#include <wiring/m5_unit_unified_wiring.hpp>
class CardNfc {
public:
  DeviceStorage &storage;
  CardService &cards;
  RtcClock &clock;
  tc::Receiver receiver;
  bool active = false, sending = false, done = false, failed = false;
  uint32_t completedAt = 0, openedAt = 0;
  String error;
  uint8_t mutualLeg = 0;
  uint64_t mutualToken = 0;
  uint64_t exchangeToken() const { return sending ? mutualToken : receiver.mutualToken; }
  uint32_t paintUs = 0, paintCount = 0;
  void recordPaint(uint32_t us) { paintUs += us; ++paintCount; }
  tc::TransferProgress progress(uint32_t now) const {
    tc::TransferProgress p;
    p.transferred = sending ? sendOffset : receiver.offset;
    p.total = sending && firstContactAt ? outgoing.size() : sending ? 0 : receiver.size;
    if (failed) p.stage = tc::TransferStage::Failed;
    else if (done) p.stage = tc::exchangeUiReady(sending, finishSent,
        now - completedAt, now - finishSentAt) ? tc::TransferStage::Complete : tc::TransferStage::Saving;
    else if (sending) {
      if (!firstContactAt) p.stage = tc::TransferStage::Waiting;
      else if (!connected || fieldRecovery) p.stage = tc::TransferStage::Reconnecting;
      else if (sendPhase == 6) p.stage = tc::TransferStage::Connecting;
      else if (sendPhase >= 3) p.stage = tc::TransferStage::Saving;
      else if (sendPhase < 2) p.stage = tc::TransferStage::Connecting;
      else p.stage = tc::TransferStage::Transferring;
    } else if (receiver.commitPending || receiver.clockPending || receiver.status == tc::Verifying ||
               (receiver.size && receiver.offset == receiver.size))
      p.stage = tc::TransferStage::Saving;
    else if (receiver.id)
      p.stage = now - receiver.lastActivity > 1200 ? tc::TransferStage::Reconnecting : tc::TransferStage::Transferring;
    else if (peerSeen) p.stage = tc::TransferStage::Connecting;
    return p;
  }
  bool readyToClose() const {
    return done && tc::exchangeUiReady(sending, finishSent, millis() - completedAt,
                                       millis() - finishSentAt);
  }
  CardNfc(DeviceStorage &s, CardService &c, RtcClock &r)
      : storage(s), cards(c), clock(r), emulation(unit, *this), reader(unit) {}
  bool open(bool phone, uint8_t leg = 0, uint64_t token = 0) {
    stop();
    mutualLeg = leg; mutualToken = token;
    receiver.beginSession(phone,
#if TOUCH_CARD_PAPER_MONO
                          1
#else
                          2
#endif
                          , leg, token
    );
    cacheInternal = storage.internal.freeBytes();
    cacheSd = storage.sd.freeBytes();
    if (!initialize(true)) {
      receiver.cancel();
      return false;
    }
    const uint64_t mac = ESP.getEfuseMac();
    uint8_t uid[7] = {4};
    for (int i = 0; i < 6; i++)
      uid[i + 1] = mac >> (i * 8);
    if (!picc.emulate(m5::nfc::a::Type::MIFARE_Ultralight_EV1_1, uid, 7))
      return fail(tc::tr("NFC初期化に失敗しました", "NFC initialization failed"));
    memcpy(memory, uid, 3);
    memory[3] = 0x88 ^ uid[0] ^ uid[1] ^ uid[2];
    memcpy(memory + 4, uid + 3, 4);
    memory[8] = uid[3] ^ uid[4] ^ uid[5] ^ uid[6];
    if (!emulation.begin(picc, memory, sizeof(memory)))
      return fail(tc::tr("NFC待機を開始できません", "Cannot start NFC listening"));
    active = true;
    openedAt = millis();
    receiver.lastActivity = openedAt;
    return true;
  }
  bool prepareOwn() {
    tc::Bytes own;
    JsonDocument d;
    if (!storage.own.load("own", own) || !tc::parse(own, d) ||
        !tc::profileValid(d["profile"]))
      return fail(tc::tr("先に自分の名刺を設定してください", "Set up your own card first"));
    JsonDocument card;
    tc::exchangeCard(d, card);
    outgoing = tc::encode(card);
    compactOutgoing.clear();
    if (!tc::packCard(card, compactOutgoing))
      return fail(tc::tr("名刺の画像データを確認してください", "Check your card image"));
    Serial.printf("[tc.nfc.payload] compact=%u metadata=%lu jpeg=%lu exchange_avatar=%u\n",
        unsigned(compactOutgoing.size()), (unsigned long)tc::le32(compactOutgoing.data() + 4),
        (unsigned long)tc::le32(compactOutgoing.data() + 8), !d["exchangeAvatar"].isNull());
    if (outgoing.size() > tc::MaxPayload)
      return fail(tc::tr("名刺のデータ量が上限を超えています", "Card exceeds the size limit"));
    return true;
  }
  bool sendOwn(uint8_t leg = 0, uint64_t token = 0) {
    stop();
    mutualLeg = leg; mutualToken = token;
    if (leg == 1 && !mutualToken) {
      mutualToken = uint64_t(esp_random()) | (uint64_t(esp_random()) << 32);
      if (!mutualToken) mutualToken = 1;
    }
    if (leg == 2 && !mutualToken) return fail(tc::tr("交換の相手を確認できません", "Cannot verify the exchange peer"));
    if (!prepareOwn()) return false;
    transferId = esp_random();
    if (!transferId)
      transferId = 1;
    outgoingCrc = tc::crc(outgoing.data(), outgoing.size());
    sendOffset = 0;
    sendPhase = 0;
    connected = false;
    negotiated = supportsFinish = false;
    outgoingEncoding = tc::JsonEncoding;
    sendChunk = 128;
    retries = retryTotal = 0;
    dataFailures = 0;
    verifyingSince = 0;
    firstContactAt = dataStartedAt = dataEndedAt = 0;
    dataIoUs = dataIoCalls = 0;
    if (!initialize(false))
      return false;
    active = sending = true;
    openedAt = millis();
    lastCommand = 0;
    fieldRecovery = 0;
    peerSeen = false;
    discoveryStartedAt = openedAt;
    return true;
  }
  void stop() {
    mutualLeg = 0; mutualToken = 0;
    if (active) {
      if (sending) {
        reader.deactivate();
        unit.disableField();
      }
      else
        emulation.end();
    }
    active = sending = false;
    done = failed = false;
    receiver.cancel();
    destination = nullptr;
    completedAt = 0;
    finishSent = false;
    finishSentAt = 0;
    error = "";
    fieldRecovery = 0;
    peerSeen = false;
  }
  void tick() {
    if (!active)
      return;
    units.update();
    if (sending) {
      sendTick();
      return;
    }
    const bool pendingBefore = receiver.commitPending || receiver.clockPending;
    emulation.update();
    // Return to the UI once after COMMIT before synchronous flash writes. The
    // application can paint Saving outside the timing-critical RF callback.
    if (!pendingBefore && (receiver.commitPending || receiver.clockPending)) return;
    if (receiver.commitPending) {
      receiver.commitPending = false;
      if (tc::crc(receiver.buffer.data(), receiver.size) !=
          receiver.expectedCrc)
        receiver.status = tc::BadCrc;
      else {
        tc::Bytes bytes(receiver.buffer.begin(),
                        receiver.buffer.begin() + receiver.size);
        const auto saveStart = millis();
        storage.internal.performance = {};
        storage.sd.performance = {};
        tc::Bytes json;
        if (receiver.encoding == tc::CompactCardEncoding && !tc::unpackCard(bytes, json))
          receiver.status = tc::InvalidData;
        else
          receiver.status = cards.apply(receiver.target,
                                        receiver.encoding == tc::CompactCardEncoding ? json : bytes,
                                        destination, destinationGeneration);
        Serial.printf("[tc.nfc.rx] wire_bytes=%lu encoding=%u save_ms=%lu status=%u\n",
                      (unsigned long)receiver.size, receiver.encoding,
                      (unsigned long)(millis() - saveStart), unsigned(receiver.status));
        storage.internal.logPerformance("internal");
        if (storage.sd.ready) storage.sd.logPerformance("sd");
      }
      if (receiver.status == tc::Stored) {
        done = true;
        completedAt = millis();
      } else {
        failed = true;
        completedAt = millis();
        error = statusText(receiver.status);
      }
    }
    if (receiver.clockPending) {
      receiver.clockPending = false;
      if (clock.sync(receiver.unixSeconds, receiver.utcOffset)) {
        receiver.status = tc::Stored;
        done = true;
        completedAt = millis();
      } else {
        receiver.status = tc::StorageError;
        failed = true;
        completedAt = millis();
        error = tc::tr("時計を保存できません", "Cannot save the time");
      }
    }
    if (receiver.id && receiver.status == tc::Receiving && destination &&
        (!destination->ready ||
         destinationGeneration != destination->generation)) {
      receiver.status = tc::MediaLost;
      failed = true;
      completedAt = millis();
      error = statusText(tc::MediaLost);
    }
    if (!done &&
        millis() - receiver.lastActivity > (receiver.id ? 120000 : 60000)) {
      failed = true;
      error = tc::tr("待機時間を超えました", "Waiting timed out");
      emulation.end();
      active = false;
    }
  }
  static String statusText(tc::Status s) {
    switch (s) {
    case tc::NoSpace:
      return tc::tr("保存容量が不足しています", "Not enough storage");
    case tc::MediaLost:
      return tc::tr("保存先が取り外されました", "Storage was removed");
    case tc::Conflict:
      return tc::tr("名刺の更新世代が競合しています", "Card revision conflict");
    case tc::WrongSession:
      return tc::tr("更新と名刺受信の待機が異なります", "Select the correct receive mode");
    case tc::Unsupported:
      return tc::tr("画像の用途・端末サイズを確認してください", "Check image purpose and device size");
    case tc::InvalidData:
      return tc::tr("名刺または画像の形式が不正です", "Invalid card or image format");
    case tc::StorageError:
      return tc::tr("受信した名刺の保存に失敗しました（16）", "Could not save received card (16)");
    default:
      return tc::tr("保存・通信に失敗しました（", "Storage or transfer failed (") + String(int(s)) + tc::tr("）", ")");
    }
  }

private:
  m5::unit::UnitUnified units;
  m5::unit::UnitNFC unit;
  class Emulation : public m5::nfc::EmulationLayerA {
    m5::unit::UnitNFC &device;
    CardNfc &owner;

  public:
    Emulation(m5::unit::UnitNFC &u, CardNfc &o)
        : m5::nfc::EmulationLayerA(u), device(u), owner(o) {}
    State receive_callback(const uint8_t *p, uint32_t n) override {
      if (n < 4 || p[0] != 0x54 || p[1] != 0x43)
        return EmulationLayerA::receive_callback(p, n);
      owner.peerSeen = true;
      uint32_t before = owner.receiver.id;
      uint8_t response[32] = {};
      size_t count = owner.receiver.handle(p, n, response, millis());
      if (owner.receiver.id != before &&
          owner.receiver.status == tc::Receiving) {
        owner.failed = owner.done = false;
        owner.finishSent = false;
        owner.error = "";
        owner.destination = nullptr;
        const auto needed = (owner.receiver.encoding == tc::CompactCardEncoding
                                 ? (uint64_t(owner.receiver.size) + 2) / 3 * 4
                                 : uint64_t(owner.receiver.size)) + 4096;
        // Use cached free space only: no filesystem operations in RF callback.
        using namespace touchcard;
        Usage internal{owner.storage.internal.ready &&
                               owner.storage.internal.writable
                           ? MediaState::Ready
                           : MediaState::Error,
                       owner.cacheInternal, 0, BoardStorage::kInternalReserve};
        Usage sd{owner.storage.sd.ready && owner.storage.sd.writable
                     ? MediaState::Ready
                     : MediaState::Error,
                 owner.cacheSd, 0, 65536};
        auto preference =
            owner.receiver.target == 5 && !owner.storage.internalOnly
                ? Destination::Auto
                : Destination::InternalOnly;
        auto chosen = chooseDestination(preference, needed, internal, sd);
        if (chosen)
          owner.destination = *chosen == Medium::Sd ? &owner.storage.sd
                                                    : &owner.storage.internal;
        if (owner.destination)
          owner.destinationGeneration = owner.destination->generation;
        else {
          owner.receiver.status = tc::NoSpace;
          response[4] = tc::NoSpace;
          owner.failed = true;
          owner.completedAt = millis();
          owner.error = statusText(tc::NoSpace);
        }
      }
      bool transmitted = count && device.nfcaEmulationTransmit(response, count);
      if (p[3] != 3 && p[3] != 4)
        Serial.printf("[tc.nfc.rx.command] cmd=%u status=%u reply=%u\n",
                      p[3], response[4], transmitted);
      if (transmitted && p[3] == 8 && response[4] == tc::Stored &&
          owner.receiver.finishRequested) {
        owner.finishSent = true;
        owner.finishSentAt = millis();
      }
      return transmitted ? State::Active : State::Idle;
    }
  } emulation;
  m5::nfc::NFCLayerA reader;
  m5::nfc::a::PICC picc;
  uint8_t memory[80] = {0,    0,    0,    0,    0,    0,    0,    0,    0,
                        0xA3, 0,    0,    0xE1, 0x10, 6,    0,    3,    0x10,
                        0xD1, 1,    0x0C, 0x54, 2,    0x65, 0x6E, 0x50, 0x61,
                        0x70, 0x65, 0x72, 0x4D, 0x4F, 0x4E, 0x4F, 0xFE, 0};
  bool registered = false, connected = false;
  bool peerSeen = false;
  ArduinoIO *destination = nullptr;
  uint64_t destinationGeneration = 0, cacheInternal = 0, cacheSd = 0;
  tc::Bytes outgoing, compactOutgoing;
  bool negotiated = false, supportsFinish = false, finishSent = false;
  uint8_t outgoingEncoding = tc::JsonEncoding;
  size_t sendChunk = 128;
  uint32_t finishSentAt = 0, firstContactAt = 0, dataStartedAt = 0,
           dataEndedAt = 0, retryTotal = 0;
  uint32_t transferId = 0, outgoingCrc = 0, sendOffset = 0, lastCommand = 0;
  uint32_t dataIoUs = 0, dataIoCalls = 0;
  int sendPhase = 0, retries = 0;
  unsigned dataFailures = 0;
  uint32_t verifyingSince = 0;
  uint8_t fieldRecovery = 0;
  uint32_t fieldChangedAt = 0, discoveryStartedAt = 0;
  void recoverField(const char *reason) {
    // A peer left in HALT/ACTIVE may ignore REQA indefinitely. Drop the RF
    // field, then let its listener process field-off before restarting.
    connected = false;
    retries = 0;
    if (!unit.disableField()) {
      fail(tc::tr("NFCの再接続準備に失敗しました", "Cannot prepare NFC reconnection"));
      return;
    }
    fieldRecovery = 1;
    fieldChangedAt = millis();
    Serial.printf("[tc.nfc.recover] reason=%s phase=%d offset=%lu\n",
                  reason, sendPhase, (unsigned long)sendOffset);
  }
  bool fail(const String &message) {
    failed = true;
    error = message;
    return false;
  }
  bool initialize(bool target) {
    paintUs = paintCount = 0;
#if TOUCH_CARD_PAPER_MONO
    auto &ioe = M5.getIOExpander(0);
    auto pin = m5::M5IOE1_Class::gpio4;
    ioe.setHighImpedance(pin, false);
    ioe.setDirection(pin, true);
    ioe.digitalWrite(pin, false);
    delay(20);
    ioe.digitalWrite(pin, true);
    delay(100);
#endif
    auto config = unit.config();
    config.emulation = target;
    config.mode = m5::nfc::NFC::A;
    config.using_irq = false;
    unit.config(config);
    Serial.printf("[tc.nfc.init] role=%s begin=%s\n",
                  target ? "receive" : "send", registered ? "unit" : "units");
    if (!registered) {
      if (!m5::unit::wiring::i2cClass(units, unit, M5.In_I2C))
        return fail(tc::tr("NFCのI2C接続に失敗しました", "NFC I2C connection failed"));
      registered = true;
      if (!units.begin())
        return fail(tc::tr("NFCを初期化できません", "Cannot initialize NFC"));
    } else if (!unit.begin())
      return fail(tc::tr("NFCを再初期化できません", "Cannot reinitialize NFC"));
    // begin() already configures the selected mode and enables the reader RF
    // field. Reconfiguring it here calls initial_field_on() a second time,
    // which fails with "Already tx_en". Do not configure either mode twice.
    if (target) {
      if (!unit.writeExternalFieldDetectorActivationThreshold(0x11) ||
          !unit.writeExternalFieldDetectorDeactivationThreshold(0))
        return fail(tc::tr("NFC受信感度を設定できません", "Cannot set NFC sensitivity"));
    }
    Serial.printf("[tc.nfc.init] ready role=%s\n", target ? "receive" : "send");
    return true;
  }
  void sendTick() {
    if (done || failed)
      return;
    if (millis() - openedAt > 120000) {
      fail(tc::tr("送信時間を超えました。再試行してください", "Transfer timed out. Try again."));
      return;
    }
    // Non-blocking off/settle intervals keep cancellation responsive. Keep
    // the transfer ID/payload intact so HELLO + BEGIN can resume safely.
    if (fieldRecovery == 1) {
      if (millis() - fieldChangedAt < 20) return;
      if (!unit.enableField()) {
        fail(tc::tr("NFCの再接続に失敗しました", "NFC reconnection failed"));
        return;
      }
      fieldRecovery = 2;
      fieldChangedAt = millis();
      return;
    }
    if (fieldRecovery == 2) {
      if (millis() - fieldChangedAt < 10) return;
      fieldRecovery = 0;
      discoveryStartedAt = millis();
    }
    if (millis() - lastCommand < (sendPhase == 4 ? 30U : 2U))
      return;
    lastCommand = millis();
    if (!connected) {
      if (millis() - discoveryStartedAt >= 1000) {
        recoverField("discovery_timeout");
        return;
      }
      std::vector<m5::nfc::a::PICC> found;
      // The vector overload defaults to a 1000 ms scan, which starves touch
      // polling and loses short cancel taps. Resume discovery next loop.
      if (!reader.detect(found, 10U))
        return;
      if (found.size() != 1) {
        reader.deactivate();
        return;
      }
      if (!reader.reactivate(found.front())) {
        recoverField("reactivate_failed");
        return;
      }
      Serial.printf("[tc.nfc.tx] selected peers=%u\n", unsigned(found.size()));
      connected = true;
      if (!firstContactAt) firstContactAt = millis();
      sendPhase = 0;
      return;
    }
    uint8_t command[253] = {0x54, 0x43, 2, 1};
    size_t length = 4;
    if (sendPhase == 1) {
      command[3] = 2;
      tc::put32(command + 4, transferId);
      command[8] = 1;
      command[9] = 5;
      command[10] = outgoingEncoding;
      tc::put32(command + 15, outgoing.size());
      tc::put32(command + 19, outgoingCrc);
      length = 23;
    }
    if (sendPhase == 2) {
      command[3] = 3;
      tc::put32(command + 4, transferId);
      tc::put32(command + 8, sendOffset);
      if (!dataStartedAt) dataStartedAt = millis();
      size_t count = std::min(sendChunk, outgoing.size() - sendOffset);
      command[12] = count;
      memcpy(command + 13, outgoing.data() + sendOffset, count);
      length = 13 + count;
    }
    if (sendPhase == 3) {
      command[3] = 5;
      tc::put32(command + 4, transferId);
      tc::put32(command + 8, outgoingCrc);
      length = 12;
    }
    if (sendPhase == 4) {
      command[3] = 4;
      tc::put32(command + 4, transferId);
      length = 8;
    }
    if (sendPhase == 5) {
      command[3] = 8;
      tc::put32(command + 4, transferId);
      length = 8;
    }
    if (sendPhase == 6) {
      command[3] = 9;
      tc::put32(command + 4, uint32_t(mutualToken));
      tc::put32(command + 8, uint32_t(mutualToken >> 32));
      command[12] = mutualLeg;
      length = 13;
    }
    uint8_t response[64] = {};
    uint16_t size = sizeof(response);
    const uint32_t ioStarted = micros();
    bool exchanged = unit.nfcaTransceive(response, size, command, length,
                                       tc::cardResponseTimeout(sendPhase), 15);
    if (sendPhase == 2) { dataIoUs += micros() - ioStarted; ++dataIoCalls; }
    if (sendPhase == 5) {
      // STORED was already verified. FINISH only releases the peer's UI grace
      // period; a lost FINISH response must not turn a durable save into failure.
      completeSend();
      return;
    }
    const uint16_t wireSize = size;
    const bool validFrame = exchanged && tc::nfcAPayloadSize(response, size);
    if (!validFrame ||
        size < 13 || response[0] != 0x54 || response[1] != 0x43 ||
        response[2] != 2 || response[3] != (command[3] | 0x80)) {
      Serial.printf("[tc.nfc.tx.retry] phase=%d exchanged=%u valid_frame=%u wire_bytes=%u retry=%d\n",
                    sendPhase, exchanged, validFrame, wireSize, retries + 1);
      ++retryTotal;
      if (sendPhase == 3) {
        // COMMIT may have arrived even when its reply was lost. Query durable
        // status before retrying it or resetting the field during flash I/O.
        if (!verifyingSince) verifyingSince = millis();
        sendPhase = 4;
        return;
      }
      if (sendPhase == 4 && verifyingSince &&
          tc::waitForCardSave(millis() - verifyingSince)) return;
      if (sendPhase == 2)
        sendChunk = tc::cardRetryChunk(sendChunk, ++dataFailures);
      ++retries;
      if (retries >= 3) {
        recoverField("command_retries");
      }
      return;
    }
    retries = 0;
    auto status = tc::Status(response[4]);
    if (status == tc::Verifying && !verifyingSince) verifyingSince = millis();
    if (sendPhase == 0 || sendPhase == 1 || status == tc::StorageError ||
        status == tc::Stored || status == tc::MediaLost || status == tc::NoSpace)
      Serial.printf("[tc.nfc.tx.reply] phase=%d status=%u\n", sendPhase, unsigned(status));
    if (sendPhase == 0) {
      if (status != tc::Ok || size < 26 || response[25] != 2) {
        fail(tc::tr("相手側で名刺の『受け取る』を開いてください", "Select Receive on the other device."));
        return;
      }
      const bool peerMutual = response[23] & tc::MutualCapability;
      if (bool(mutualLeg) != peerMutual ||
          (mutualLeg && (size < 27 || response[26] != mutualLeg ||
                         !(response[23] & tc::FinishCapability)))) {
        fail(tc::tr("両方で「交換」を選び、先に渡す側と受け取る側を分けてください", "Select Exchange on both devices, with opposite first roles."));
        return;
      }
      size_t commandMax = tc::le16(response + 15);
      size_t advertisedChunk = tc::le16(response + 17);
      if (commandMax <= 13 || !advertisedChunk || !(response[23] & 1)) {
        fail(tc::tr("相手のNFC転送設定に対応していません", "Unsupported peer transfer settings"));
        return;
      }
      if (!negotiated) {
        if (response[23] & tc::CompactCardCapability) {
          outgoing.swap(compactOutgoing);
          outgoingEncoding = tc::CompactCardEncoding;
          outgoingCrc = tc::crc(outgoing.data(), outgoing.size());
          sendChunk = 240;
        }
        negotiated = true;
      } else if (outgoingEncoding == tc::CompactCardEncoding &&
                 !(response[23] & tc::CompactCardCapability)) {
        fail(tc::tr("相手が変わりました。送信をやり直してください", "Peer changed. Start again."));
        return;
      }
      sendChunk = std::min({sendChunk, advertisedChunk, commandMax - 13, size_t(240)});
      supportsFinish = response[23] & tc::FinishCapability;
      if (outgoing.size() > tc::le32(response + 19)) {
        fail(tc::tr("相手が受け取れる名刺容量を超えています", "Card exceeds the peer's size limit"));
        return;
      }
      sendPhase = mutualLeg ? 6 : 1;
      return;
    }
    if (sendPhase == 6) {
      Serial.printf("[tc.nfc.pair] leg=%u status=%u wire_bytes=%u payload_bytes=%u\n",
                    mutualLeg, unsigned(status), wireSize, size);
      if (status != tc::Ok || size != 22 || response[21] != mutualLeg ||
          (uint64_t(tc::le32(response + 13)) | (uint64_t(tc::le32(response + 17)) << 32)) != mutualToken) {
        fail(tc::tr("交換の相手が一致しません。最初からやり直してください", "Exchange peer mismatch. Start again."));
        return;
      }
      sendPhase = 1;
      return;
    }
    if (tc::le32(response + 5) != transferId) {
      fail(tc::tr("転送IDが一致しません", "Transfer ID mismatch"));
      return;
    }
    if (status == tc::Stored) {
      if (tc::le32(response + 9) != outgoing.size()) {
        fail(tc::tr("保存確認のデータ量が一致しません", "Stored size mismatch"));
        return;
      }
      if (supportsFinish) sendPhase = 5;
      else completeSend();
      return;
    }
    if (status != tc::Ok && status != tc::Accepted && status != tc::Receiving &&
        status != tc::Verifying && status != tc::Busy &&
        status != tc::BadOffset) {
      fail(statusText(status));
      return;
    }
    if (status == tc::Busy)
      return;
    if (sendPhase == 1 || sendPhase == 2 ||
        (sendPhase == 4 && (status == tc::Receiving || status == tc::BadOffset))) {
      uint32_t next = tc::le32(response + 9);
      if (next > outgoing.size()) {
        fail(tc::tr("不正な転送位置です", "Invalid transfer offset"));
        return;
      }
      if (next > sendOffset) dataFailures = 0;
      sendOffset = next;
      if (next == outgoing.size() && !dataEndedAt) dataEndedAt = millis();
      sendPhase = next == outgoing.size() ? 3 : 2;
    } else
      sendPhase = 4;
  }
  void completeSend() {
    done = true;
    completedAt = millis();
    Serial.printf("[tc.nfc.tx] wire_bytes=%u encoding=%u chunk=%u retries=%lu data_ms=%lu selected_to_complete_ms=%lu\n",
                  unsigned(outgoing.size()), outgoingEncoding, unsigned(sendChunk),
                  (unsigned long)retryTotal,
                  (unsigned long)(dataStartedAt ? dataEndedAt - dataStartedAt : 0),
                  (unsigned long)(completedAt - firstContactAt));
    Serial.printf("[tc.nfc.cost] data_io_us=%lu data_calls=%lu paint_us=%lu paints=%lu\n",
        (unsigned long)dataIoUs, (unsigned long)dataIoCalls,
        (unsigned long)paintUs, (unsigned long)paintCount);
  }
};
