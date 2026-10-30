#include <assert.h>
#include <iostream>
#include <string>
#include "../firmware/esp8266_4relay/relay_names.h"

bool valid(const std::string& name) { return relayapp::validRelayName(name.data(), name.size()); }
int main() {
  assert(valid(u8"Đèn phòng khách"));
  assert(valid(u8"Quạt trần"));
  assert(valid(std::string(24, 'a')));
  assert(!valid(std::string(25, 'a')));
  assert(!valid(""));
  assert(!valid("   "));
  assert(!valid("a\nb"));
  assert(!valid(std::string("a\0b", 3)));
  assert(!valid(std::string("\xC0\xAF", 2))); // overlong UTF-8
  assert(!valid(std::string("\xED\xA0\x80", 3))); // surrogate
  assert(!valid(std::string("\xF4\x90\x80\x80", 4))); // out of range
  std::string emoji;
  for (int i = 0; i < 24; ++i) emoji += "\xF0\x9F\x92\xA1";
  assert(valid(emoji));
  assert(!valid(emoji + "a"));
  assert(valid("<b>Light</b>")); // Names are rendered as text, never HTML.
  std::cout << "PASS: UTF-8 device names, limits and invalid input\n";
}
