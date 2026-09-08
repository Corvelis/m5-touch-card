#pragma once
#include <cstdint>

namespace tc {
// Only the idle, dark PaperMono may stop its CPU. All filesystem writes are
// synchronous; the catalog and NFC state below cover work spanning loop calls.
struct PaperSleepState {
  bool paper = false, dark = false;
  bool nfc = false, exchange = false, pendingSave = false;
  bool scanning = false, displayBusy = false, buttonHeld = false;
  bool statusDue = false;
};
constexpr bool canPaperSleep(const PaperSleepState &s) {
  return s.paper && s.dark && !s.nfc && !s.exchange && !s.pendingSave &&
         !s.scanning && !s.displayBusy && !s.buttonHeld && !s.statusDue;
}

// Wake just after the next minute boundary, not 60 seconds after drawing.
// The small margin avoids waking before the boundary due to timer rounding.
constexpr uint64_t paperMinuteSleepUs(bool clockValid, uint64_t unixUs,
                                     uint32_t uptimeMs) {
  constexpr uint64_t minute = 60000000;
  const uint64_t phase = clockValid ? unixUs % minute
                                   : uint64_t(uptimeMs % 60000) * 1000;
  return minute - phase + 2000;
}

// A GPIO wake is a press, while M5 BtnB.wasClicked() is a later release.
// Consume that release so waking does not immediately turn the light off again.
struct PaperWakeButton {
  bool pending = false, consumingRelease = false;
  void wake() { pending = true; }
  bool toggle(bool clicked, bool rawPressed, bool debouncedPressed) {
    if (pending) {
      pending = false;
      consumingRelease = true;
      return true;
    }
    if (consumingRelease) {
      if (!rawPressed && !debouncedPressed) consumingRelease = false;
      return false;
    }
    return clicked;
  }
};

struct PaperSleepRetry {
  bool failed = false;
  uint32_t failedAt = 0;
  bool ready(uint32_t now) const {
    return !failed || uint32_t(now - failedAt) >= 5000;
  }
  void failure(uint32_t now) { failed = true; failedAt = now; }
  void success() { failed = false; }
};

enum class PaperWake { Skipped, Timer, Button, Other, Error };
struct PaperSleepResult { PaperWake wake; int error = 0; };

// Platform-injected sequence: fault-injection tested on the host, used with
// the ESP-IDF adapter on PaperMono. No reset, power-rail cuts or full draw.
template <typename Platform>
PaperSleepResult enterPaperSleep(Platform &hw, uint64_t windowUs) {
  if (hw.buttonPressed()) return {PaperWake::Button};
  const uint64_t started = hw.nowUs();
  hw.quietDisplay();
  auto finish = [&](PaperSleepResult result) {
    hw.disarm();
    hw.resumeDisplay();
    return result;
  };
  if (hw.displayBusy()) return finish({PaperWake::Skipped});
  const uint64_t elapsed = hw.nowUs() - started;
  if (elapsed >= windowUs) return finish({PaperWake::Skipped});
  int error = hw.armTimer(windowUs - elapsed);
  if (!error) error = hw.armButton();
  if (error) return finish({PaperWake::Error, error});
  // Catch a press during display/RTC preparation. Level wake also catches a
  // press after this check, including a pulse released before loop resumes.
  if (hw.buttonPressed()) return finish({PaperWake::Button});
  error = hw.sleep();
  if (hw.buttonPressed() || (!error && hw.wakeCause() == PaperWake::Button))
    return finish({PaperWake::Button});
  return finish(error ? PaperSleepResult{PaperWake::Error, error}
                      : PaperSleepResult{hw.wakeCause()});
}
} // namespace tc
