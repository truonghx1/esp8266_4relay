#pragma once
#include <ArduinoJson.h>
#include "scheduler.h"

// Firebase can return numeric child keys as either an object or an array.
// Validate the entire snapshot before changing any physical output.
inline bool readFirebaseRelays(JsonVariantConst root, uint8_t& on, uint8_t& missing) {
  on = 0; missing = 0;
  if (!(root.isNull() || root.is<JsonObjectConst>() || root.is<JsonArrayConst>())) return false;
  for (uint8_t i = 0; i < relayapp::RELAY_COUNT; ++i) {
    const char key[] = {static_cast<char>('1' + i), '\0'};
    JsonVariantConst value = root.is<JsonArrayConst>() ? root[i + 1] : root[key];
    if (value.isNull()) { missing |= 1U << i; continue; }
    if (!value.is<int>() || (value.as<int>() != 0 && value.as<int>() != 1)) return false;
    if (value.as<int>() == 1) on |= 1U << i;
  }
  return true;
}
