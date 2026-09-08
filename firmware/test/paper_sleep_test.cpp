#include "PaperSleep.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

struct FakeSleep {
  uint64_t time = 1000000, preparation = 7000, timer = 0;
  bool busy = false;
  int timerError = 0, buttonError = 0, sleepError = 0;
  tc::PaperWake cause = tc::PaperWake::Timer;
  int pressedOnCall = 0, buttonReads = 0;
  int quiet = 0, restore = 0, cleanup = 0, timerArms = 0, buttonArms = 0, sleeps = 0;
  bool buttonPressed() { return ++buttonReads == pressedOnCall; }
  uint64_t nowUs() const { return time; }
  void quietDisplay() { ++quiet; time += preparation; }
  bool displayBusy() const { return busy; }
  void resumeDisplay() { ++restore; }
  int armTimer(uint64_t us) { ++timerArms; timer = us; return timerError; }
  int armButton() { ++buttonArms; return buttonError; }
  int sleep() { ++sleeps; time += timer; return sleepError; }
  tc::PaperWake wakeCause() const { return cause; }
  void disarm() { ++cleanup; }
};

int main(int argc, char **argv) {
  using namespace tc;
  // Exhaustive eligibility: every in-flight activity, a held wake button, a
  // pending minute redraw, a lit display and StackChan all prevent sleeping.
  for (unsigned bits = 0; bits < 512; ++bits) {
    PaperSleepState s{bool(bits & 1), bool(bits & 2), bool(bits & 4),
      bool(bits & 8), bool(bits & 16), bool(bits & 32), bool(bits & 64),
      bool(bits & 128), bool(bits & 256)};
    assert(canPaperSleep(s) == (bits == 3));
  }
  constexpr uint64_t minuteUs = 60000000;
  for (uint64_t phase = 0; phase < minuteUs; phase += 997) {
    const auto duration = paperMinuteSleepUs(true, 1700000040ULL * 1000000 + phase, 42);
    assert(duration == minuteUs - phase + 2000);
    assert(duration > 2000 && duration <= minuteUs + 2000);
  }
  assert(paperMinuteSleepUs(true, 59999999, 0) == 2001);
  assert(paperMinuteSleepUs(true, 60000000, 0) == 60002000);
  // Clock unset and millis wrapping: still bounded, never a huge delay.
  for (uint32_t ms : {0U, 12345U, 59999U, 60000U, UINT32_MAX, 3U}) {
    assert(paperMinuteSleepUs(false, 0, ms) == uint64_t(60000 - ms % 60000) * 1000 + 2000);
  }

  PaperWakeButton button;
  assert(button.toggle(true, false, false)); // Normal click turns light off.
  assert(!button.toggle(false, false, false)); // Timer wake does not toggle.
  button.wake();
  assert(button.toggle(false, true, true)); // GPIO press turns light on once.
  assert(!button.toggle(false, true, true)); // Holding cannot toggle again.
  assert(!button.toggle(false, false, true)); // Debouncer still held.
  assert(!button.toggle(true, false, false)); // Consume its release click.
  assert(button.toggle(true, false, false)); // Next deliberate click works.
  button.wake();
  assert(button.toggle(false, false, false)); // Short pulse ended before loop.
  assert(!button.toggle(true, false, false));
  assert(button.toggle(true, false, false));

  PaperSleepRetry retry;
  assert(retry.ready(0));
  retry.failure(UINT32_MAX - 2000);
  assert(!retry.ready(2000));
  assert(retry.ready(3000));
  retry.success();
  assert(retry.ready(0));

  FakeSleep normal;
  assert(enterPaperSleep(normal, minuteUs).wake == PaperWake::Timer);
  assert(normal.timer == minuteUs - normal.preparation);
  assert(normal.sleeps == 1 && normal.quiet == 1 && normal.restore == 1 && normal.cleanup == 1);
  FakeSleep gpio;
  gpio.cause = PaperWake::Button; // GPIO pulse already released on return.
  assert(enterPaperSleep(gpio, minuteUs).wake == PaperWake::Button);
  for (int at : {1, 2, 3}) {
    FakeSleep press;
    press.pressedOnCall = at;
    assert(enterPaperSleep(press, minuteUs).wake == PaperWake::Button);
    assert(press.sleeps == (at == 3 ? 1 : 0));
    assert(press.restore == (at == 1 ? 0 : 1));
    assert(press.cleanup == press.restore);
  }
  for (int failure = 0; failure < 3; ++failure) {
    FakeSleep hw;
    if (failure == 0) hw.timerError = 101;
    if (failure == 1) hw.buttonError = 102;
    if (failure == 2) hw.sleepError = 103;
    const auto result = enterPaperSleep(hw, minuteUs);
    assert(result.wake == PaperWake::Error && result.error == 101 + failure);
    assert(hw.sleeps == (failure == 2 ? 1 : 0));
    assert(hw.restore == 1 && hw.cleanup == 1);
  }
  FakeSleep stillBusy;
  stillBusy.busy = true;
  assert(enterPaperSleep(stillBusy, minuteUs).wake == PaperWake::Skipped);
  assert(!stillBusy.sleeps && stillBusy.restore == 1 && stillBusy.cleanup == 1);
  FakeSleep crossedMinute;
  crossedMinute.preparation = 10000;
  assert(enterPaperSleep(crossedMinute, 2001).wake == PaperWake::Skipped);
  assert(!crossedMinute.timerArms && !crossedMinute.sleeps && crossedMinute.restore == 1);

  // Integration contracts, explicitly not a physical GPIO/display test.
  assert(argc == 3);
  auto read = [](const char *path) {
    std::ifstream file(path);
    assert(file);
    return std::string(std::istreambuf_iterator<char>(file), {});
  };
  const auto main = read(argv[1]), adapter = read(argv[2]);
  const auto finishStart = main.find("void finishLoop()");
  const auto finishEnd = main.find("} // namespace", finishStart);
  const auto finish = main.substr(finishStart, finishEnd - finishStart);
  assert(finish.find("#if TOUCH_CARD_PAPER_MONO") < finish.find("tc::enterPaperSleep"));
  for (auto required : {"nfc.active || screen == Screen::Nfc", "mutual.active || mutual.switching",
       "nfc.receiver.commitPending || nfc.receiver.clockPending", "storage.catalog.scanning",
       "tc::minuteStatusDue", "tc::canPaperSleep(state)", "paperWakeButton.wake()",
       "lastClock = millis() - 1000"}) assert(finish.find(required) != std::string::npos);
  assert(finish.find("render()") == std::string::npos);
  assert(finish.find("nfc.stop()") == std::string::npos);
  assert(adapter.find("esp_light_sleep_start()") != std::string::npos);
  assert(adapter.find("GPIO_NUM_3") != std::string::npos);
  assert(adapter.find("GPIO_INTR_LOW_LEVEL") != std::string::npos);
  assert(adapter.find("M5.Display.powerSaveOn()") == std::string::npos);
  assert(adapter.find("M5.Display.powerSaveOff()") == std::string::npos);
  assert(adapter.find("M5.Display.setPowerSave(") == std::string::npos);
  assert(adapter.find("M5.Display.sleep()") == std::string::npos);
  assert(adapter.find("M5.Display.wakeup()") == std::string::npos);
  assert(adapter.find("esp_deep_sleep_start") == std::string::npos);
  assert(adapter.find("setBrightness") == std::string::npos);
  const auto loop = main.substr(main.find("void loop()"));
  assert(loop.find("paperWakeButton.toggle") < loop.find("screenBlanker.toggle"));
  auto early = loop.find("if (!tc::minuteStatusDue");
  assert(loop.find("finishLoop();", early) < loop.find("return;", early));
  assert(loop.rfind("finishLoop();") > loop.find("nfc.tick();"));
  const auto minuteStart = loop.find("bool hourlyClean = false;");
  const auto minuteCode = loop.substr(minuteStart);
  for (auto required : {"paperHourlyRefresh.observe", "now.value.tm_year", "now.value.tm_yday",
      "now.value.tm_hour", "now.value.tm_min", "nfc.active || screen == Screen::Nfc",
      "mutual.active || mutual.switching", "nfc.receiver.commitPending",
      "nfc.receiver.clockPending", "storage.catalog.scanning || M5.Display.displayBusy()",
      "paperHourlyRefresh.due(maintenanceBusy)", "&& !hourlyClean)",
      "hourlyClean ? epd_mode_t::epd_quality : epd_mode_t::epd_fastest",
      "tc::paintPaperMinute", "paperHourlyRefresh.completed()", "hourly_clean=%u"})
    assert(minuteCode.find(required) != std::string::npos);
  assert(minuteCode.find("paperHourlyRefresh.observe") < minuteCode.find("if (!tc::minuteStatusDue"));
  assert(minuteCode.find("tc::paintPaperMinute") < minuteCode.find("paperHourlyRefresh.completed()"));
  assert(minuteCode.find("setBrightness") == std::string::npos);
  assert(minuteCode.find("fillScreen") == std::string::npos);
  assert(minuteCode.find("loadOwn()") == std::string::npos);
  const auto render = main.substr(main.find("void render() {"), main.find("void waitNfc") - main.find("void render() {"));
  assert(render.find("if (refresh == tc::PaperRefresh::Clean)") < render.find("paperHourlyRefresh.completed()"));
  std::cout << "PaperMono light sleep: eligibility, timing, wake clicks and cleanup PASS\n";
}
