#pragma once
#include <cassert>
#include <cstdint>

// Host substitutes for the hardware boundaries only. PaperLightSleep.h and
// enterPaperSleep are the production implementation, not reimplemented here.
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
enum gpio_num_t { GPIO_NUM_3 = 3 };
enum gpio_int_type_t { GPIO_INTR_LOW_LEVEL = 4 };
enum esp_sleep_wakeup_cause_t {
  ESP_SLEEP_WAKEUP_UNDEFINED, ESP_SLEEP_WAKEUP_TIMER, ESP_SLEEP_WAKEUP_GPIO
};
namespace paper_sleep_test {
struct Hardware {
  uint64_t now = 0, timerUs = 0;
  bool buttonLow = false, busy = false;
  bool timerArmed = false, buttonArmed = false, gpioWakeArmed = false;
  bool historyValid = true;
  unsigned sleeps = 0, displayPowerChanges = 0;
  int timerError = 0, buttonError = 0, gpioError = 0, sleepError = 0;
  esp_sleep_wakeup_cause_t wake = ESP_SLEEP_WAKEUP_TIMER;
};
inline Hardware hw;
struct FakeDisplay {
  bool displayBusy() const { return hw.busy; }
  // SSD1677_4Gray::setPowerSave invalidates history even when RAM is retained.
  void powerSaveOn() { ++hw.displayPowerChanges; hw.historyValid = false; }
  void powerSaveOff() { ++hw.displayPowerChanges; hw.historyValid = false; }
};
struct Device { FakeDisplay Display; };
} // namespace paper_sleep_test
inline paper_sleep_test::Device M5;

inline int gpio_get_level(gpio_num_t pin) {
  assert(pin == GPIO_NUM_3);
  return !paper_sleep_test::hw.buttonLow;
}
inline int64_t esp_timer_get_time() { return paper_sleep_test::hw.now; }
inline esp_err_t esp_sleep_enable_timer_wakeup(uint64_t us) {
  auto &s = paper_sleep_test::hw;
  if (!s.timerError) { s.timerUs = us; s.timerArmed = true; }
  return s.timerError;
}
inline esp_err_t gpio_wakeup_enable(gpio_num_t pin, gpio_int_type_t level) {
  assert(pin == GPIO_NUM_3 && level == GPIO_INTR_LOW_LEVEL);
  auto &s = paper_sleep_test::hw;
  if (!s.buttonError) s.buttonArmed = true;
  return s.buttonError;
}
inline esp_err_t esp_sleep_enable_gpio_wakeup() {
  auto &s = paper_sleep_test::hw;
  if (!s.gpioError) s.gpioWakeArmed = true;
  return s.gpioError;
}
inline esp_err_t gpio_wakeup_disable(gpio_num_t pin) {
  assert(pin == GPIO_NUM_3);
  paper_sleep_test::hw.buttonArmed = false;
  return ESP_OK;
}
inline esp_err_t esp_sleep_disable_wakeup_source(esp_sleep_wakeup_cause_t source) {
  assert(source == ESP_SLEEP_WAKEUP_TIMER || source == ESP_SLEEP_WAKEUP_GPIO);
  auto &s = paper_sleep_test::hw;
  if (source == ESP_SLEEP_WAKEUP_TIMER) s.timerArmed = false;
  else s.gpioWakeArmed = false;
  return ESP_OK;
}
inline esp_err_t esp_light_sleep_start() {
  auto &s = paper_sleep_test::hw;
  assert(s.timerArmed && s.buttonArmed && s.gpioWakeArmed);
  ++s.sleeps;
  if (!s.sleepError) s.now += s.timerUs;
  return s.sleepError;
}
inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() {
  return paper_sleep_test::hw.wake;
}
