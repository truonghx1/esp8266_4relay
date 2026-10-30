#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <ArduinoJson.h>
#include "../firmware/esp8266_4relay/firebase_relays.h"
using namespace relayapp;
size_t serializationBudget = static_cast<size_t>(-1);
class String : public std::string {
  size_t pos = 0;
 public:
  using std::string::string;
  using std::string::operator=;
  String() = default;
  String(const std::string& s) : std::string(s) {}
  explicit String(unsigned int n) : std::string(std::to_string(n)) {}
  explicit String(int n) : std::string(std::to_string(n)) {}
  size_t write(uint8_t c) { return write(&c, 1); }
  size_t write(const uint8_t* data, size_t n) {
    n = std::min(n, serializationBudget); serializationBudget -= n;
    append(reinterpret_cast<const char*>(data), n); return n;
  }
  int read() { return pos < size() ? static_cast<unsigned char>((*this)[pos++]) : -1; }
  size_t readBytes(char* dest, size_t n) {
    size_t count = std::min(n, size() - pos); memcpy(dest, data() + pos, count); pos += count; return count;
  }
};
struct { template<class... T> void printf(const char*, T...) {} } Serial;
RelayLogic relayLogic;
String relayNames[4] = {"One", "Two", "Three", "Four"};
String firebaseStatus, lastFirebaseReport;
uint32_t lastFirebaseReportAt = 0, nowMs = 20000;
uint8_t cloudDirty = 0, firebaseMissingChannel = 0;
bool cloudApplying = false, firebaseReportPending = false;
uint8_t scheduled = 0;
uint32_t millis() { return nowMs; }
void updateRelays() {
  const uint8_t previous = relayLogic.output();
  relayLogic.evaluate(scheduled);
  if (!cloudApplying) cloudDirty |= previous ^ relayLogic.output();
}
struct Call { String path, method, body; bool conditional; };
std::vector<Call> calls;
String cloud = "[null,0,1,0,1]";
int reportCode = 200;
int createCode = 200, scheduleDuringRead = -1;
int firebaseRequest(const String& path, const char* method, const String& body, String& response, bool conditional = false) {
  calls.push_back({path, method, body, conditional});
  response = std::string(method) == "GET" ? cloud : String("{}");
  if (std::string(method) == "GET" && scheduleDuringRead >= 0) {
    scheduled = scheduleDuringRead; scheduleDuringRead = -1;
  }
  if (std::string(method) == "PUT") return createCode;
  return path.empty() ? reportCode : 200;
}
bool firebaseOK(int code) { return code >= 200 && code < 300; }
// Generated directly from connectivity.h by run_firebase_sync_test.cjs.
#include "../build/firebase_sync_under_test.h"
void tick() {
  size_t count = calls.size(); syncFirebase();
  assert(calls.size() <= count + 1); // No second TLS handshake in one call frame.
  nowMs += 2000;
}
void reset() {
  relayLogic = RelayLogic{}; cloudDirty = 0; scheduled = 0;
  firebaseMissingChannel = 0; firebaseReportPending = false;
  lastFirebaseReport = ""; calls.clear(); reportCode = 200; createCode = 200;
}
int main() {
  relayLogic.setManual(0, true);
  tick(); assert(calls.back().method == "GET" && relayLogic.output() == 10);
  assert(firebaseReportPending);
  tick(); assert(calls.back().method == "PATCH" && calls.back().path.empty());
  DynamicJsonDocument report(1024);
  assert(!deserializeJson(report, calls.back().body.c_str()));
  assert(report["reported"]["1"].as<int>() == 0);
  assert(!firebaseReportPending);
  tick(); assert(calls.back().method == "GET" && !firebaseReportPending);
  cloud = "[null,1,1,0,1]";
  tick(); assert(firebaseReportPending && relayLogic.output() == 11);
  reportCode = 401;
  tick(); assert(!firebaseReportPending);
  cloud = "[null,0,1,0,1]";
  tick(); assert(calls.back().method == "GET" && relayLogic.output() == 10);
  relayLogic.setManual(0, true); cloudDirty = 1;
  tick(); assert(calls.back().path == "/relays" && calls.back().body == "{\"1\":1}");
  assert(!cloudDirty && !firebaseReportPending);
  cloud = "null";
  tick(); assert(calls.back().method == "GET" && firebaseMissingChannel == 1);
  tick(); assert(calls.back().method == "PUT" && calls.back().path == "/relays/1");
  assert(firebaseMissingChannel == 0);

  // Readable channels still work when another channel is absent and its
  // conditional creation is denied. No failed PUT can monopolize the poller.
  reset(); cloud = "{\"1\":1,\"3\":0,\"4\":0}";
  tick(); assert(relayLogic.output() == 1 && firebaseMissingChannel == 2);
  createCode = 403;
  tick(); assert(calls.back().conditional && firebaseMissingChannel == 0);
  tick(); assert(calls.back().method == "PATCH" && calls.back().path.empty());
  cloud = "{\"1\":0,\"3\":1,\"4\":0}";
  tick(); assert(calls.back().method == "GET" && relayLogic.output() == 4);
  createCode = 412;
  tick(); assert(firebaseMissingChannel == 0);

  // No channel changes when an otherwise partial snapshot has invalid data.
  reset(); relayLogic.setManual(0, true);
  cloud = "{\"1\":0,\"2\":2}";
  tick(); assert(relayLogic.output() == 1 && firebaseMissingChannel == 0);

  // A schedule edge during the blocking GET takes local priority and is
  // published before this stale cloud value can turn the channel back off.
  reset(); cloud = "[null,0,0,0,0]"; scheduleDuringRead = 1;
  tick(); assert(relayLogic.output() == 1 && cloudDirty == 1 && !firebaseReportPending);
  tick(); assert(calls.back().method == "PATCH" && calls.back().body == "{\"1\":1}");
  assert(cloudDirty == 0);

  // OOM during String serialization must not send a truncated patch or clear
  // its pending channels. Likewise, an incomplete report signature is unsafe.
  reset(); relayLogic.setManual(0, true); cloudDirty = 1;
  serializationBudget = 4;
  tick(); assert(calls.empty() && cloudDirty == 1);
  String body, signature; serializationBudget = 0;
  assert(!buildFirebaseReport(body, signature));
  serializationBudget = static_cast<size_t>(-1);
  tick(); assert(calls.back().body == "{\"1\":1}" && cloudDirty == 0);
  std::cout << "PASS: one HTTP request per sync, reports/init, missing-channel read/write failures, malformed snapshots, schedule edges during GET, serialization OOM, local priority\n";
}
