#include "PaperLightSleep.h"
#include <cassert>
#include <iostream>

static void assertRetained(const PaperLightSleep &adapter) {
  const auto &s = paper_sleep_test::hw;
  assert(s.historyValid && s.displayPowerChanges == 0);
  assert(!s.timerArmed && !s.buttonArmed && !s.gpioWakeArmed);
  assert(!adapter.timerArmed && !adapter.buttonArmed && !adapter.gpioWakeArmed);
}

int main() {
  using namespace tc;
  auto &s = paper_sleep_test::hw;
  PaperLightSleep adapter;
  constexpr uint64_t minute = 60000000;
  // A day of minute-aligned cycles through the real hardware adapter must
  // neither cycle panel power nor invalidate the differential baseline.
  s.now = 1700000040ULL * 1000000 + 13000000;
  for (unsigned i = 0; i < 1440; ++i) {
    const auto duration = paperMinuteSleepUs(true, s.now, 0);
    assert(enterPaperSleep(adapter, duration).wake == PaperWake::Timer);
    assert(s.sleeps == i + 1 && s.now % minute == 2000);
    assertRetained(adapter);
    s.now += 350000; // Time spent doing the regional minute update.
  }
  // Already pressed, pulse/release wake, other wake and busy display paths.
  s = {};
  s.buttonLow = true;
  assert(enterPaperSleep(adapter, minute).wake == PaperWake::Button);
  assert(s.sleeps == 0);
  assertRetained(adapter);
  for (const auto cause : {ESP_SLEEP_WAKEUP_GPIO, ESP_SLEEP_WAKEUP_UNDEFINED}) {
    s = {};
    s.wake = cause;
    const auto expected = cause == ESP_SLEEP_WAKEUP_GPIO ? PaperWake::Button : PaperWake::Other;
    assert(enterPaperSleep(adapter, minute).wake == expected);
    assert(s.sleeps == 1);
    assertRetained(adapter);
  }
  s = {};
  s.busy = true;
  assert(enterPaperSleep(adapter, minute).wake == PaperWake::Skipped);
  assert(s.sleeps == 0);
  assertRetained(adapter);
  // Partial wake-source setup and sleep failures still leave display history
  // intact and remove only the wake sources that were actually armed.
  for (unsigned failure = 0; failure < 4; ++failure) {
    s = {};
    if (failure == 0) s.timerError = 101;
    if (failure == 1) s.buttonError = 102;
    if (failure == 2) s.gpioError = 103;
    if (failure == 3) s.sleepError = 104;
    const auto result = enterPaperSleep(adapter, minute);
    assert(result.wake == PaperWake::Error && result.error == int(101 + failure));
    assert(s.sleeps == (failure == 3 ? 1U : 0U));
    assertRetained(adapter);
  }
  std::cout << "PaperMono production sleep adapter: retained display history, minute/button wakes and failure cleanup PASS\n";
}
