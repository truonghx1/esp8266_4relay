#pragma once
#include <ArduinoJson.h>
#include <string.h>

// JSON escaping can expand each of the 95 credential bytes to six bytes.
// The serialized file bound is separate from the parsed JSON memory capacity.
constexpr size_t WIFI_FILE_LIMIT = 640;

inline bool validWifiCredentials(JsonVariantConst ssid, JsonVariantConst password) {
  if (!ssid.is<const char*>() || !password.is<const char*>()) return false;
  const JsonString name = ssid.as<JsonString>();
  const JsonString secret = password.as<JsonString>();
  return name.size() >= 1 && name.size() <= 32 &&
         (secret.size() == 0 || (secret.size() >= 8 && secret.size() <= 63)) &&
         strlen(name.c_str()) == name.size() && strlen(secret.c_str()) == secret.size();
}
