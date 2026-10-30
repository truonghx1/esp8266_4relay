#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include "../firmware/esp8266_4relay/firebase_relays.h"

// Minimal host adapter for Arduino's String/Stream. The transport below honors
// ESP8266 StreamSend.cpp's writable-capacity contract, unlike a fetch mock.
class String {
 public:
  std::string value;
  bool failAllocation = false;
  size_t length() const { return value.size(); }
  bool concat(const char* bytes, size_t count) {
    if (failAllocation) return false;
    value.append(bytes, count); return true;
  }
};
class Stream {
 public:
  virtual ~Stream() = default;
  virtual int availableForWrite() { return 0; }
  virtual bool outputCanTimeout() { return true; }
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t* bytes, size_t count) {
    size_t n = 0; while (n < count && write(bytes[n])) ++n; return n;
  }
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  virtual void flush() = 0;
};
#include "../firmware/esp8266_4relay/firebase_body.h"

bool deliver(Stream& sink, const std::string& bytes, size_t chunkSize) {
  size_t offset = 0;
  while (offset < bytes.size()) {
    const int capacity = sink.availableForWrite();
    if (capacity <= 0) return false; // The old receiver stopped here, even on HTTP 200.
    const size_t amount = std::min({static_cast<size_t>(capacity), chunkSize, bytes.size() - offset});
    const size_t written = sink.write(reinterpret_cast<const uint8_t*>(bytes.data() + offset), amount);
    if (!written) return false;
    offset += written;
  }
  return true;
}

int main() {
  relayapp::RelayLogic relay;
  relay.setManual(0, true); // Local ON is already active when web sends OFF.
  for (const std::string payload : {"{\"1\":0,\"2\":1,\"3\":0,\"4\":1}", "[null,0,1,0,1]"}) {
    for (const size_t chunk : {1U, 3U, 4096U}) {
      FirebaseBody receiver;
      assert(deliver(receiver, payload, chunk));
      assert(receiver.body.value == payload);
      ArduinoJson::DynamicJsonDocument doc(512);
      assert(!deserializeJson(doc, receiver.body.value));
      uint8_t desired, missing;
      assert(readFirebaseRelays(doc.as<JsonVariantConst>(), desired, missing));
      assert(desired == 10 && missing == 0);
      for (uint8_t i = 0; i < 4; ++i)
        if (((relay.output() >> i) & 1U) != ((desired >> i) & 1U))
          relay.setManual(i, (desired & (1U << i)) != 0);
      relay.evaluate(0);
      assert(relay.output() == 10); // Web OFF overrides the previous local ON.
      relay.setManual(0, true);
    }
  }
  ArduinoJson::DynamicJsonDocument doc(512);
  uint8_t desired, missing;
  for (const char* payload : {"{\"1\":true}", "{\"1\":\"0\"}", "{\"1\":2}", "42"}) {
    assert(!deserializeJson(doc, payload));
    assert(!readFirebaseRelays(doc.as<JsonVariantConst>(), desired, missing));
  }
  assert(!deserializeJson(doc, "null"));
  assert(readFirebaseRelays(doc.as<JsonVariantConst>(), desired, missing) && missing == 15);
  assert(!deserializeJson(doc, "{\"1\":1}"));
  assert(readFirebaseRelays(doc.as<JsonVariantConst>(), desired, missing) && missing == 14 && desired == 1);
  FirebaseBody full;
  assert(deliver(full, std::string(4096, 'x'), 128));
  assert(full.availableForWrite() == 0);
  assert(!deliver(full, "x", 1));
  assert(full.write(static_cast<uint8_t>('x')) == 0 && full.overflow);
  FirebaseBody tooLarge;
  assert(!deliver(tooLarge, std::string(4097, 'x'), 128));
  assert(tooLarge.body.length() == 4096);
  FirebaseBody oom;
  oom.body.failAllocation = true;
  assert(!deliver(oom, "{}", 2));
  assert(oom.allocationFailed && oom.availableForWrite() == 0);
  std::cout << "PASS: HTTP writable capacity, chunked body, Firebase object/array, web OFF after local ON, malformed data, size limit and allocation failure\n";
}
