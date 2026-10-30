#include <cassert>
#include <iostream>
#include <string>
#include "../firmware/esp8266_4relay/wifi_credentials.h"

int main() {
  DynamicJsonDocument doc(1024);
  for (const char* json : {
      "{\"ssid\":\"Home\",\"password\":\"12345678\"}",
      "{\"ssid\":\"Open\",\"password\":\"\"}"}) {
    assert(!deserializeJson(doc, json));
    assert(validWifiCredentials(doc["ssid"], doc["password"]));
  }
  for (const char* json : {
      "{\"ssid\":\"Home\",\"password\":\"1234567\"}",
      "{\"ssid\":\"Home\\u0000Other\",\"password\":\"12345678\"}",
      "{\"ssid\":\"Home\",\"password\":\"12345678\\u0000Other\"}",
      "{\"ssid\":\"\",\"password\":\"12345678\"}",
      "{\"ssid\":42,\"password\":\"12345678\"}", "{}"}) {
    assert(!deserializeJson(doc, json));
    assert(!validWifiCredentials(doc["ssid"], doc["password"]));
  }
  doc.clear();
  doc["ssid"] = std::string(32, '\x01');
  doc["password"] = std::string(63, '\x02');
  assert(validWifiCredentials(doc["ssid"], doc["password"]));
  std::string serialized;
  serializeJson(doc, serialized);
  assert(serialized.size() > 384 && serialized.size() <= WIFI_FILE_LIMIT);
  // Loading the serialized representation uses the same decoded validation.
  assert(!deserializeJson(doc, serialized));
  assert(validWifiCredentials(doc["ssid"], doc["password"]));
  doc["ssid"] = std::string(33, 'a');
  assert(!validWifiCredentials(doc["ssid"], doc["password"]));
  doc["ssid"] = "Home"; doc["password"] = std::string(64, 'a');
  assert(!validWifiCredentials(doc["ssid"], doc["password"]));
  std::cout << "PASS: saved/API Wi-Fi validation, open networks, embedded NUL, byte limits and escaped credential round trip\n";
}
