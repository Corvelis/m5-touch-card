#pragma once
#if TOUCH_CARD_PAPER_MONO
#include <PaperSleep.h>
#include <M5Unified.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <sys/time.h>

// PaperMono C153: Button B is active-low GPIO3 (M5Unified / official pin map).
// Keep its existing input/pull configuration; never repurpose PMIC power keys.
struct PaperLightSleep {
  static constexpr gpio_num_t buttonPin = GPIO_NUM_3;
  bool timerArmed = false, buttonArmed = false, gpioWakeArmed = false;
  bool buttonPressed() const { return gpio_get_level(buttonPin) == 0; }
  uint64_t nowUs() const { return esp_timer_get_time(); }
  // Keep these hooks deliberately empty. The pinned SSD1677_4Gray driver
  // invalidates its optical baseline and Mode 2 RAM face on BOTH powerSave
  // transitions; the next fastest draw then falls back to a full refresh.
  // Only the CPU sleeps here. Keep the panel's power/history state untouched,
  // including on timer/GPIO setup failures and button wake.
  void quietDisplay() const {}
  bool displayBusy() const { return M5.Display.displayBusy(); }
  void resumeDisplay() const {}
  int armTimer(uint64_t us) {
    const auto err = esp_sleep_enable_timer_wakeup(us);
    timerArmed = err == ESP_OK;
    return err;
  }
  int armButton() {
    auto err = gpio_wakeup_enable(buttonPin, GPIO_INTR_LOW_LEVEL);
    buttonArmed = err == ESP_OK;
    if (err == ESP_OK) {
      err = esp_sleep_enable_gpio_wakeup();
      gpioWakeArmed = err == ESP_OK;
    }
    return err;
  }
  int sleep() { return esp_light_sleep_start(); }
  tc::PaperWake wakeCause() const {
    switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_GPIO: return tc::PaperWake::Button;
    case ESP_SLEEP_WAKEUP_TIMER: return tc::PaperWake::Timer;
    default: return tc::PaperWake::Other;
    }
  }
  void disarm() {
    // Clean up only our two sources, including partially failed setup.
    if (buttonArmed) gpio_wakeup_disable(buttonPin);
    if (gpioWakeArmed) esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    if (timerArmed) esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    timerArmed = buttonArmed = gpioWakeArmed = false;
  }
};
#endif
