#include <assert.h>
#include <iostream>
#include "../firmware/esp8266_4relay/scheduler.h"
using namespace relayapp;

Schedule make(uint8_t relays, uint8_t days, uint16_t start, uint16_t end) {
  Schedule s;
  s.used = true; s.enabled = true; s.relays = relays;
  s.days = days; s.start = start; s.end = end;
  return s;
}

int main() {
  Schedule list[MAX_SCHEDULES];
  list[0] = make(3, 1, 480, 600); // Monday 08:00..10:00, relay 1+2.
  assert(activeMask(list, 0, 479) == 0);
  assert(activeMask(list, 0, 480) == 3);
  assert(activeMask(list, 0, 599) == 3);
  assert(activeMask(list, 0, 600) == 0);
  assert(activeMask(list, 1, 480) == 0);
  list[1] = make(6, 1, 540, 660); // Overlap keeps relay 2 on until 11:00.
  assert(activeMask(list, 0, 550) == 7);
  assert(activeMask(list, 0, 600) == 6);
  list[1].enabled = false;
  assert(activeMask(list, 0, 550) == 3);

  Schedule overnight = make(1, 1U << 6, 1320, 120); // Sunday -> Monday.
  assert(isActive(overnight, 6, 1320));
  assert(isActive(overnight, 0, 0));
  assert(isActive(overnight, 0, 119));
  assert(!isActive(overnight, 0, 120));
  assert(!isActive(overnight, 6, 60));
  assert(!isActive(overnight, 0, 1320));
  overnight.end = 0;
  assert(isActive(overnight, 6, 1439));
  assert(!isActive(overnight, 0, 0));
  overnight.start = overnight.end;
  assert(!validSchedule(overnight));

  // Exhaustively compare each minute of a week against independently built
  // intervals, including wrap to next week and two consecutive selected days.
  const uint16_t starts[] = {0, 1, 480, 1320, 1439};
  const uint16_t ends[] = {0, 1, 120, 600, 1439};
  for (auto start : starts) for (auto end : ends) {
    if (start == end) continue;
    Schedule s = make(1, 0x61, start, end); // Mon, Sat, Sun.
    for (int t = 0; t < 10080; ++t) {
      bool expected = false;
      for (int day = 0; day < 7; ++day) {
        if (!(s.days & (1U << day))) continue;
        const int a = day * 1440 + start;
        const int b = day * 1440 + end + (end < start ? 1440 : 0);
        expected |= (t >= a && t < b) || (t + 10080 >= a && t + 10080 < b);
      }
      assert(isActive(s, t / 1440, t % 1440) == expected);
    }
  }

  RelayLogic logic;
  logic.evaluate(3);
  logic.setManual(0, false);
  assert(logic.output() == 2 && logic.manual() == 1);
  logic.evaluate(7); // Other channel changes do not erase relay 1 override.
  assert(logic.output() == 6 && logic.manual() == 1);
  logic.evaluate(6); // Relay 1 interval ends, its override is cleared.
  assert(logic.output() == 6 && logic.manual() == 0);
  logic.evaluate(7);
  assert(logic.output() == 7);
  logic.setManual(3, true);
  assert(logic.output() == 15);
  logic.automatic(3);
  assert(logic.output() == 7);
  logic.setManual(0, false);
  logic.evaluate(7, true);
  assert(logic.output() == 7 && logic.manual() == 0);
  logic.evaluate(0);
  logic.setManual(0, true);
  logic.evaluate(0); // No schedule: manual command persists.
  assert(logic.output() == 1);
  logic.evaluate(1, true); // First clock sync puts relays back in auto.
  assert(logic.output() == 1 && logic.manual() == 0);

  relayapp::Clock clock;
  assert(!clock.valid());
  clock.sync(1704067200ULL, 0xFFFFFF00U); // 2024-01-01, Monday 00:00 UTC.
  uint8_t day; uint16_t minute;
  clock.local(420, day, minute);
  assert(day == 0 && minute == 420);
  clock.local(-60, day, minute);
  assert(day == 6 && minute == 1380);
  clock.tick(744); // 1000 ms across rollover.
  assert(clock.epoch() == 1704067201ULL);
  clock.tick(1244);
  assert(clock.epoch() == 1704067201ULL);
  clock.tick(1744);
  assert(clock.epoch() == 1704067202ULL);
  clock.sync(4102444799ULL, 2000); // Beyond signed 32-bit Unix time.
  clock.tick(3000);
  assert(clock.epoch() == 4102444800ULL);
  clock.local(0, day, minute);
  assert(day == 4 && minute == 0); // 2100-01-01, Friday.
  std::cout << "PASS: weekly schedules, overnight, overlaps, manual control, clock and millis rollover\n";
}
