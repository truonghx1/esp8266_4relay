#pragma once
#include <stdint.h>

namespace relayapp {
constexpr uint8_t RELAY_COUNT = 4;
constexpr uint8_t MAX_SCHEDULES = 10;

struct Schedule {
  bool used = false;
  bool enabled = true;
  uint8_t relays = 0;  // Bits 0..3 = relay 1..4.
  uint8_t days = 0;    // Bits 0..6 = Monday..Sunday; day when interval STARTS.
  uint16_t start = 0;  // Minutes since midnight, inclusive.
  uint16_t end = 0;    // Exclusive. Equal start/end is not accepted.
};

inline bool validSchedule(const Schedule& s) {
  return s.relays > 0 && s.relays <= 15 && s.days > 0 && s.days <= 127 &&
         s.start < 1440 && s.end < 1440 && s.start != s.end;
}

inline bool isActive(const Schedule& s, uint8_t weekday, uint16_t minute) {
  if (!s.used || !s.enabled || !validSchedule(s) || weekday > 6 || minute >= 1440)
    return false;
  const bool today = (s.days & (1U << weekday)) != 0;
  if (s.start < s.end) return today && minute >= s.start && minute < s.end;
  const uint8_t yesterday = (weekday + 6) % 7;
  return (today && minute >= s.start) ||
         ((s.days & (1U << yesterday)) && minute < s.end);
}

inline uint8_t activeMask(const Schedule* schedules, uint8_t weekday, uint16_t minute) {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < MAX_SCHEDULES; ++i)
    if (isActive(schedules[i], weekday, minute)) mask |= schedules[i].relays;
  return mask;
}

// No RTC or Internet in AP-only mode. Tick from the main loop, even without clients.
// Unsigned subtraction handles the millis() wrap approximately every 49.7 days.
class Clock {
 public:
  void sync(uint64_t epoch, uint32_t now) {
    epoch_ = epoch;
    lastMillis_ = now;
    remainder_ = 0;
    valid_ = true;
  }
  void tick(uint32_t now) {
    const uint32_t delta = now - lastMillis_;
    lastMillis_ = now;
    if (!valid_) return;
    const uint64_t total = static_cast<uint64_t>(remainder_) + delta;
    epoch_ += total / 1000;
    remainder_ = total % 1000;
  }
  bool valid() const { return valid_; }
  uint64_t epoch() const { return epoch_; }
  void local(int offsetMinutes, uint8_t& weekday, uint16_t& minute) const {
    const int64_t seconds = static_cast<int64_t>(epoch_) + offsetMinutes * 60;
    // Epochs accepted by the API are >= 2024, so local seconds are positive.
    weekday = (seconds / 86400 + 3) % 7; // 1970-01-01 was Thursday.
    minute = (seconds % 86400) / 60;
  }
 private:
  bool valid_ = false;
  uint64_t epoch_ = 0;
  uint32_t lastMillis_ = 0;
  uint16_t remainder_ = 0;
};

class RelayLogic {
 public:
  void evaluate(uint8_t scheduled, bool resetManual = false) {
    // Manual commands last until the aggregate schedule changes for that relay.
    const uint8_t changed = scheduled ^ previous_;
    manual_ &= static_cast<uint8_t>(~changed);
    if (resetManual) manual_ = 0;
    previous_ = scheduled;
    output_ = (scheduled & ~manual_) | (manualValues_ & manual_);
  }
  void setManual(uint8_t relay, bool on) {
    if (relay >= RELAY_COUNT) return;
    const uint8_t bit = 1U << relay;
    manual_ |= bit;
    if (on) manualValues_ |= bit;
    else manualValues_ &= ~bit;
    evaluate(previous_);
  }
  void automatic(uint8_t relay) {
    if (relay >= RELAY_COUNT) return;
    manual_ &= ~(1U << relay);
    evaluate(previous_);
  }
  uint8_t output() const { return output_; }
  uint8_t manual() const { return manual_; }
 private:
  uint8_t previous_ = 0;
  uint8_t manual_ = 0;
  uint8_t manualValues_ = 0;
  uint8_t output_ = 0;
};
} // namespace relayapp
