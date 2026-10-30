#include <cassert>
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <ArduinoJson.h>
#include "../firmware/esp8266_4relay/scheduler.h"
#include "../firmware/esp8266_4relay/relay_names.h"
using namespace relayapp;

class String : public std::string {
 public:
  using std::string::string;
  using std::string::operator=;
  String() = default;
  String(const std::string& s) : std::string(s) {}
  int indexOf(char c) const { const size_t n = find(c); return n == npos ? -1 : static_cast<int>(n); }
  String substring(size_t start, size_t end) const { return substr(start, end - start); }
  void trim() {
    const size_t first = find_first_not_of(" \t\n\r\f\v");
    *this = first == npos ? "" : substr(first, find_last_not_of(" \t\n\r\f\v") - first + 1);
  }
  bool equalsIgnoreCase(const char* s) const {
    if (size() != strlen(s)) return false;
    for (size_t i = 0; i < size(); ++i)
      if (std::tolower(static_cast<unsigned char>((*this)[i])) != std::tolower(static_cast<unsigned char>(s[i]))) return false;
    return true;
  }
  size_t write(uint8_t c) { push_back(static_cast<char>(c)); return 1; }
  size_t write(const uint8_t* data, size_t count) { append(reinterpret_cast<const char*>(data), count); return count; }
};

struct File {
  std::string* content = nullptr;
  size_t position = 0;
  bool shortWrite = false;
  explicit operator bool() const { return content != nullptr; }
  size_t size() const { return content ? content->size() : 0; }
  int read() { return content && position < content->size() ? static_cast<unsigned char>((*content)[position++]) : -1; }
  size_t readBytes(char* dest, size_t count) {
    const size_t actual = std::min(count, size() - position);
    memcpy(dest, content->data() + position, actual); position += actual; return actual;
  }
  size_t write(uint8_t c) { return write(&c, 1); }
  size_t write(const uint8_t* data, size_t count) {
    if (!content || shortWrite) return 0;
    content->append(reinterpret_cast<const char*>(data), count); return count;
  }
  void flush() {}
  void close() {}
};
struct {
  std::map<std::string, std::string> files;
  bool failOpen = false, failRename = false, shortWrite = false;
  bool exists(const char* path) const { return files.count(path) != 0; }
  File open(const char* path, const char* mode) {
    File f;
    if (failOpen || (mode[0] == 'r' && !exists(path))) return f;
    if (mode[0] == 'w') files[path].clear();
    f.content = &files[path]; f.shortWrite = shortWrite; return f;
  }
  bool rename(const char* from, const char* to) {
    if (failRename || !exists(from)) return false;
    files[to] = files[from]; files.erase(from); return true;
  }
  bool remove(const char* path) { return files.erase(path) != 0; }
} LittleFS;
struct {
  String body, type = "application/json", response;
  int code = 0;
  bool hasArg(const char*) const { return !body.empty(); }
  String arg(const char*) const { return body; }
  String header(const char*) const { return type; }
  void send(int c, const char*, const String& s) { code = c; response = s; }
  void sendHeader(const char*, const char*) {}
} server;
constexpr int WL_CONNECTED = 3, HIGH = 1, LOW = 0;
struct { String toString() const { return "192.168.1.2"; } } ip;
struct {
  int connection = 0;
  int status() const { return connection; }
  decltype(ip) localIP() const { return ip; }
} WiFi;
uint32_t nowMs = 0;
uint32_t millis() { return nowMs; }
const uint8_t RELAY_PINS[] = {5, 4, 14, 12};
constexpr bool RELAY_ACTIVE_LOW = false;
int gpio[16] = {};
void digitalWrite(uint8_t pin, int value) { gpio[pin] = value; }
Schedule schedules[MAX_SCHEDULES];
String relayNames[RELAY_COUNT] = {"One", "Two", "Three", "Four"};
relayapp::Clock deviceClock;
RelayLogic relayLogic;
int utcOffsetMinutes = 420;
bool storageReady = true, configHealthy = true, cloudApplying = false;
uint8_t cloudDirty = 0;
const char* CONFIG_PATH = "/config.json";
const char* TEMP_PATH = "/config.tmp";
String wifiSsid, wifiStatus, firebaseStatus;
#include "../build/firmware_api_under_test.h"

void request(const char* body) { server.body = body; server.code = 0; }
void assertState() {
  DynamicJsonDocument state(8192);
  assert(server.code == 200 && !deserializeJson(state, server.response.c_str()));
  assert(state["relays"].size() == RELAY_COUNT);
}

int main() {
  // Validation must reject requests without changing outputs or persisted data.
  request("{\"relay\":1,\"on\":true}"); server.type = "text/plain";
  handleRelay(false); assert(server.code == 415 && relayLogic.output() == 0);
  server.type = "Application/JSON; charset=utf-8";
  request("{\"relay\":1,\"on\":1}"); handleRelay(false); assert(server.code == 400);
  request("{\"relay\":1.5,\"on\":true}"); handleRelay(false); assert(server.code == 400);
  request("{\"relay\":1,\"on\":true}"); handleRelay(false);
  assertState(); assert(relayLogic.output() == 1 && gpio[5] == HIGH);

  request("{\"id\":-1,\"enabled\":true,\"relays\":1,\"days\":1,\"start\":0,\"end\":60}");
  handleSchedule(false); assertState(); assert(schedules[0].used);
  const std::string persisted = LittleFS.files[CONFIG_PATH];
  // Each flash failure preserves both the old file and active schedule.
  for (int failure = 0; failure < 3; ++failure) {
    LittleFS.failOpen = failure == 0;
    LittleFS.shortWrite = failure == 1;
    LittleFS.failRename = failure == 2;
    request("{\"id\":0,\"enabled\":true,\"relays\":8,\"days\":1,\"start\":0,\"end\":60}");
    handleSchedule(false);
    assert(server.code == 507 && schedules[0].relays == 1);
    assert(LittleFS.files[CONFIG_PATH] == persisted);
    LittleFS.failOpen = LittleFS.shortWrite = LittleFS.failRename = false;
  }
  request("{\"names\":[\"Changed\",\"Two\",\"bad\\u0000name\",\"Four\"]}");
  handleNames(); assert(server.code == 400 && relayNames[0] == "One");
  request("{\"names\":[\"  Light  \",\"Two\",\"Three\",\"Four\"]}");
  handleNames(); assertState(); assert(relayNames[0] == "Light");

  // Malformed stored schedules must be rejected atomically, not partially loaded.
  LittleFS.files[CONFIG_PATH] = "{\"version\":1,\"utcOffsetMinutes\":0,\"schedules\":["
    "{\"id\":0,\"enabled\":true,\"relays\":8,\"days\":1,\"start\":0,\"end\":60},"
    "{\"id\":0,\"enabled\":true,\"relays\":2,\"days\":1,\"start\":0,\"end\":60}]}";
  assert(!loadConfig() && schedules[0].relays == 1 && utcOffsetMinutes == 420);
  LittleFS.files[CONFIG_PATH] = persisted;
  assert(loadConfig() && relayNames[0] == "One");

  // A lost flash write must not prevent clock sync and local schedule operation.
  LittleFS.failRename = true;
  request("{\"epoch\":1704067200,\"utcOffsetMinutes\":0}"); handleTime();
  assertState(); assert(deviceClock.valid() && utcOffsetMinutes == 0 && !configHealthy);
  assert(relayLogic.output() == 1 && relayLogic.manual() == 0);
  assert(LittleFS.files[CONFIG_PATH] == persisted);
  LittleFS.failRename = false;
  request("{\"epoch\":4102444800,\"utcOffsetMinutes\":0}"); handleTime(); assert(server.code == 400);

  std::cout << "PASS: actual firmware API validation, GPIO, atomic config writes/load, names and clock under flash failure\n";
}
