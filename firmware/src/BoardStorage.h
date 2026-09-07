#pragma once
#include <LittleFS.h>
#include <esp_littlefs.h>
#include <M5Unified.h>
#include <SD.h>
#include <SD_MMC.h>
#include <SPI.h>
#include <StoragePolicy.h>
#include <utility/M5IOE1_Class.hpp>

// Hardware-specific mounts. Record transactions live in DeviceStorage/CardStore.
class BoardStorage {
 public:
  touchcard::Usage internal, sd;
  bool internalMounted = false, sdMounted = false;
  static constexpr uint64_t kInternalReserve = 1024 * 1024; // provisional headroom

  void begin() {
    internalMounted = LittleFS.begin(false); // Never format user data on an error.
    refreshInternal();
    mountSd();
  }
  void refreshInternal() {
    // Arduino totalBytes() and usedBytes() each scan the entire allocation
    // graph via esp_littlefs_info. Retrieve both in one fresh scan instead.
    // "spiffs" is the existing partition label used by LittleFS.begin().
    size_t total = 0, used = 0;
    const bool ok = internalMounted && esp_littlefs_info("spiffs", &total, &used) == ESP_OK;
    internal = ok ? touchcard::Usage{touchcard::MediaState::Ready, total, used, kInternalReserve}
                  : touchcard::Usage{touchcard::MediaState::Error};
  }
  void eject() {
#if TOUCH_CARD_PAPER_MONO
    SD_MMC.end();
#else
    SD.end();
#endif
    sdMounted = false;
    sd = {touchcard::MediaState::Ejected};
  }
  void mountSd() {
    eject();
    sd.state = touchcard::MediaState::Loading;
#if TOUCH_CARD_PAPER_MONO
    auto& ioe = M5.getIOExpander(0);
    ioe.setHighImpedance(m5::M5IOE1_Class::gpio14, false);
    ioe.setDirection(m5::M5IOE1_Class::gpio14, true);
    ioe.digitalWrite(m5::M5IOE1_Class::gpio14, true);
    ioe.setDirection(m5::M5IOE1_Class::gpio1, false);
    delay(20);
    bool notInserted = true;
    if (!ioe.getInputLevel(m5::M5IOE1_Class::gpio1, &notInserted)) {
      sd.state = touchcard::MediaState::Error;
      return;
    }
    if (notInserted) {
      sd.state = touchcard::MediaState::Absent;
      return;
    }
    if (!SD_MMC.setPins(13, 12, 11)) {
      sd.state = touchcard::MediaState::Error;
      return;
    }
    sdMounted = SD_MMC.begin("/sdcard", true, false);
    if (sdMounted) sd = {touchcard::MediaState::Ready, SD_MMC.totalBytes(), SD_MMC.usedBytes(), 65536};
#else
    // CoreS3 SD shares display SPI lines; all accesses are sequential in loop().
    SPI.begin(36, 35, 37, 4);
    sdMounted = SD.begin(4, SPI, 4000000);
    if (sdMounted) sd = {touchcard::MediaState::Ready, SD.totalBytes(), SD.usedBytes(), 65536};
#endif
    // Without insertion detection, do not pretend that a mount error means empty.
    if (!sdMounted || !sd.valid()) sd.state = touchcard::MediaState::Error;
  }
};
