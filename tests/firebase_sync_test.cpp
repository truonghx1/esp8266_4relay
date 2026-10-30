#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <ArduinoJson.h>
#include "../firmware/esp8266_4relay/firebase_relays.h"
using namespace relayapp;
class String : public std::string {
  size_t pos = 0;
 public:
  using std::string::string;
  using std::string::operator=;
  String() = default;
  String(const std::string& s) : std::string(s) {}
  explicit String(unsigned int n) : std::string(std::to_string(n)) {}
  explicit String(int n) : std::string(std::to_string(n)) {}
  size_t write(uint8_t c) { push_back(static_cast<char>(c)); return 1; }
  size_t write(const uint8_t* data, size_t n) { append(reinterpret_cast<const char*>(data), n); return n; }
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
uint32_t millis() { return nowMs; }
void updateRelays() { relayLogic.evaluate(0); }
struct Call { String path, method, body; };
std::vector<Call> calls;
String cloud = "[null,0,1,0,1]";
int reportCode = 200;
int firebaseRequest(const String& path, const char* method, const String& body, String& response, bool = false) {
  calls.push_back({path, method, body});
  response = std::string(method) == "GET" ? cloud : String("{}");
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
  std::cout << "PASS: one HTTP request per sync, deferred reports/init, web OFF, local priority, report errors do not block commands\n";
}
