#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <time.h>
#include "config.h"
#include "scheduler.h"
#include "relay_names.h"
#include "web_ui.h"
#include "firebase_ca.h"

using namespace relayapp;
ESP8266WebServer server(80);
WiFiEventHandler apClientJoined, apClientLeft;
Schedule schedules[MAX_SCHEDULES];
String relayNames[RELAY_COUNT] = {"Kênh 1", "Kênh 2", "Kênh 3", "Kênh 4"};
relayapp::Clock deviceClock;
RelayLogic relayLogic;
int utcOffsetMinutes = DEFAULT_UTC_OFFSET_MINUTES;
bool storageReady = false;
bool configHealthy = true;
const char* CONFIG_PATH = "/config.json";
const char* TEMP_PATH = "/config.tmp";
String wifiSsid, wifiPassword;
String wifiStatus = "Chưa cấu hình";
String firebaseStatus = "Chưa kết nối Wi-Fi";
uint32_t wifiAttemptAt = 0, cloudAttemptAt = 0;
bool wifiConnecting = false, wifiWasConnected = false, cloudApplying = false;
uint8_t cloudDirty = 0;

void jsonError(int code, const char* message) {
  StaticJsonDocument<256> doc;
  doc["error"] = message;
  String body;
  serializeJson(doc, body);
  server.send(code, "application/json; charset=utf-8", body);
}

bool integerIn(JsonVariantConst value, int minimum, int maximum) {
  return value.is<int>() && value.as<int>() >= minimum && value.as<int>() <= maximum;
}

bool readSchedule(JsonObjectConst obj, Schedule& out) {
  if (!obj["enabled"].is<bool>() || !integerIn(obj["relays"], 1, 15) ||
      !integerIn(obj["days"], 1, 127) || !integerIn(obj["start"], 0, 1439) ||
      !integerIn(obj["end"], 0, 1439)) return false;
  out.used = true;
  out.enabled = obj["enabled"].as<bool>();
  out.relays = obj["relays"].as<uint8_t>();
  out.days = obj["days"].as<uint8_t>();
  out.start = obj["start"].as<uint16_t>();
  out.end = obj["end"].as<uint16_t>();
  return validSchedule(out);
}

void writeSchedule(JsonObject obj, const Schedule& s, uint8_t id) {
  obj["id"] = id;
  obj["enabled"] = s.enabled;
  obj["relays"] = s.relays;
  obj["days"] = s.days;
  obj["start"] = s.start;
  obj["end"] = s.end;
}

bool readRelayNames(JsonVariantConst value, String* out) {
  if (!value.is<JsonArrayConst>() || value.size() != RELAY_COUNT) return false;
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    if (!value[i].is<const char*>()) return false;
    const JsonString raw = value[i].as<JsonString>();
    if (!validRelayName(raw.c_str(), raw.size())) return false;
    out[i] = raw.c_str();
    out[i].trim();
  }
  return true;
}

bool saveConfig(const Schedule* candidate, int offset, const String* names) {
  if (!storageReady) return false;
  DynamicJsonDocument doc(4096);
  doc["version"] = 1;
  doc["utcOffsetMinutes"] = offset;
  JsonArray savedNames = doc.createNestedArray("relayNames");
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) savedNames.add(names[i]);
  JsonArray list = doc.createNestedArray("schedules");
  for (uint8_t i = 0; i < MAX_SCHEDULES; ++i)
    if (candidate[i].used) writeSchedule(list.createNestedObject(), candidate[i], i);
  if (doc.overflowed()) return false;
  File file = LittleFS.open(TEMP_PATH, "w");
  if (!file) return false;
  const size_t expected = measureJson(doc);
  const size_t written = serializeJson(doc, file);
  file.flush();
  const size_t actual = file.size();
  file.close();
  // Rename replaces the old file atomically in LittleFS; never remove it first.
  if (written != expected || actual != expected || !LittleFS.rename(TEMP_PATH, CONFIG_PATH)) {
    LittleFS.remove(TEMP_PATH);
    return false;
  }
  configHealthy = true;
  return true;
}

bool loadConfig() {
  if (!storageReady) return false;
  if (!LittleFS.exists(CONFIG_PATH)) return true;
  File file = LittleFS.open(CONFIG_PATH, "r");
  if (!file || file.size() > 4096) return false;
  DynamicJsonDocument doc(4096);
  const DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error || !integerIn(doc["version"], 1, 1) ||
      !integerIn(doc["utcOffsetMinutes"], -720, 840) ||
      !doc["schedules"].is<JsonArray>()) return false;
  JsonArrayConst list = doc["schedules"].as<JsonArrayConst>();
  if (list.size() > MAX_SCHEDULES) return false;
  Schedule loaded[MAX_SCHEDULES];
  String loadedNames[RELAY_COUNT];
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) loadedNames[i] = relayNames[i];
  // Version 1 configurations made before device naming have no relayNames.
  if (doc.containsKey("relayNames") && !readRelayNames(doc["relayNames"], loadedNames)) return false;
  for (JsonVariantConst item : list) {
    if (!item.is<JsonObjectConst>()) return false;
    JsonObjectConst obj = item.as<JsonObjectConst>();
    if (!integerIn(obj["id"], 0, MAX_SCHEDULES - 1)) return false;
    const int id = obj["id"].as<int>();
    if (loaded[id].used || !readSchedule(obj, loaded[id])) return false;
  }
  memcpy(schedules, loaded, sizeof(schedules));
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) relayNames[i] = loadedNames[i];
  utcOffsetMinutes = doc["utcOffsetMinutes"].as<int>();
  return true;
}

void updateRelays(bool resetManual = false) {
  const uint8_t previous = relayLogic.output();
  deviceClock.tick(millis());
  uint8_t mask = 0;
  if (deviceClock.valid()) {
    uint8_t day;
    uint16_t minute;
    deviceClock.local(utcOffsetMinutes, day, minute);
    mask = activeMask(schedules, day, minute);
  }
  relayLogic.evaluate(mask, resetManual);
  if (!cloudApplying && WiFi.status() == WL_CONNECTED)
    cloudDirty |= previous ^ relayLogic.output();
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    const bool on = (relayLogic.output() & (1U << i)) != 0;
    digitalWrite(RELAY_PINS[i], (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
  }
}

void sendState() {
  updateRelays();
  DynamicJsonDocument doc(4096);
  doc["timeValid"] = deviceClock.valid();
  doc["epoch"] = deviceClock.epoch();
  doc["utcOffsetMinutes"] = utcOffsetMinutes;
  doc["storageReady"] = storageReady;
  doc["configHealthy"] = configHealthy;
  doc["maxSchedules"] = MAX_SCHEDULES;
  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["ssid"] = wifiSsid;
  wifi["connected"] = WiFi.status() == WL_CONNECTED;
  wifi["status"] = wifiStatus;
  wifi["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  wifi["firebase"] = firebaseStatus;
  JsonArray relays = doc.createNestedArray("relays");
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    JsonObject item = relays.createNestedObject();
    item["id"] = i + 1;
    item["name"] = relayNames[i];
    item["on"] = (relayLogic.output() & (1U << i)) != 0;
    item["manual"] = (relayLogic.manual() & (1U << i)) != 0;
  }
  JsonArray list = doc.createNestedArray("schedules");
  for (uint8_t i = 0; i < MAX_SCHEDULES; ++i)
    if (schedules[i].used) writeSchedule(list.createNestedObject(), schedules[i], i);
  String body;
  serializeJson(doc, body);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", body);
}

bool readBody(JsonDocument& doc) {
  // Only JSON is accepted. No CORS or OPTIONS handler: cross-origin browser
  // scripts cannot submit simple form requests to operate the relays.
  String type = server.header("Content-Type");
  int separator = type.indexOf(';');
  if (separator >= 0) type = type.substring(0, separator);
  type.trim();
  if (!type.equalsIgnoreCase("application/json")) {
    jsonError(415, "Yêu cầu Content-Type: application/json.");
    return false;
  }
  if (!server.hasArg("plain") || server.arg("plain").length() > 1024) {
    jsonError(413, "Nội dung yêu cầu trống hoặc quá dài.");
    return false;
  }
  if (deserializeJson(doc, server.arg("plain")) || !doc.is<JsonObject>()) {
    jsonError(400, "JSON không hợp lệ.");
    return false;
  }
  return true;
}

void handleTime() {
  StaticJsonDocument<256> doc;
  if (!readBody(doc)) return;
  if (!doc["epoch"].is<uint64_t>() || doc["epoch"].as<uint64_t>() < 1704067200ULL ||
      doc["epoch"].as<uint64_t>() >= 4102444800ULL ||
      !integerIn(doc["utcOffsetMinutes"], -720, 840)) {
    jsonError(400, "Giờ phải nằm trong năm 2024–2099, múi giờ UTC−12 đến UTC+14.");
    return;
  }
  const int offset = doc["utcOffsetMinutes"].as<int>();
  // Clock sync works even when flash storage fails; the UI reports that fault.
  if (offset != utcOffsetMinutes && !saveConfig(schedules, offset, relayNames)) configHealthy = false;
  const bool firstSync = !deviceClock.valid();
  utcOffsetMinutes = offset;
  deviceClock.sync(doc["epoch"].as<uint64_t>(), millis());
  updateRelays(firstSync);
  sendState();
}

void handleRelay(bool automatic) {
  StaticJsonDocument<256> doc;
  if (!readBody(doc)) return;
  if (!integerIn(doc["relay"], 1, RELAY_COUNT) || (!automatic && !doc["on"].is<bool>())) {
    jsonError(400, "Relay phải từ 1 đến 4; on phải là true hoặc false.");
    return;
  }
  updateRelays();
  const uint8_t index = doc["relay"].as<uint8_t>() - 1;
  if (automatic) relayLogic.automatic(index);
  else relayLogic.setManual(index, doc["on"].as<bool>());
  if (WiFi.status() == WL_CONNECTED) cloudDirty |= 1U << index;
  sendState();
}

void handleSchedule(bool remove) {
  StaticJsonDocument<512> doc;
  if (!readBody(doc)) return;
  if (!integerIn(doc["id"], remove ? 0 : -1, MAX_SCHEDULES - 1)) {
    jsonError(400, "ID lịch không hợp lệ.");
    return;
  }
  int id = doc["id"].as<int>();
  if (id == -1) {
    for (uint8_t i = 0; i < MAX_SCHEDULES; ++i)
      if (!schedules[i].used) { id = i; break; }
    if (id < 0) { jsonError(409, "Đã đủ 10 lịch. Hãy sửa hoặc xóa lịch cũ."); return; }
  } else if (!schedules[id].used) {
    jsonError(404, "Lịch không còn tồn tại. Hãy tải lại trang.");
    return;
  }
  Schedule candidate[MAX_SCHEDULES];
  memcpy(candidate, schedules, sizeof(schedules));
  if (remove) candidate[id] = Schedule{};
  else if (!readSchedule(doc.as<JsonObjectConst>(), candidate[id])) {
    jsonError(400, "Chọn relay, thứ và hai giờ khác nhau trong khoảng 00:00–23:59.");
    return;
  }
  if (!saveConfig(candidate, utcOffsetMinutes, relayNames)) {
    jsonError(507, "Không lưu được flash. Thay đổi chưa được áp dụng.");
    return;
  }
  memcpy(schedules, candidate, sizeof(schedules));
  updateRelays(true);
  sendState();
}

void handleNames() {
  StaticJsonDocument<1024> doc;
  if (!readBody(doc)) return;
  String candidate[RELAY_COUNT];
  if (!readRelayNames(doc["names"], candidate)) {
    jsonError(400, "Nhập đủ 4 tên, mỗi tên 1–24 ký tự, không chứa ký tự điều khiển.");
    return;
  }
  bool changed = false;
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) changed |= candidate[i] != relayNames[i];
  if (changed && !saveConfig(schedules, utcOffsetMinutes, candidate)) {
    jsonError(507, "Không lưu được tên thiết bị. Thay đổi chưa được áp dụng.");
    return;
  }
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) relayNames[i] = candidate[i];
  // Naming does not reset manual overrides or change relay-to-schedule IDs.
  sendState();
}

#include "connectivity.h"

void setup() {
  // Set the output latch before enabling output to minimize startup switching.
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    digitalWrite(RELAY_PINS[i], RELAY_ACTIVE_LOW ? HIGH : LOW);
    pinMode(RELAY_PINS[i], OUTPUT);
  }
  Serial.begin(115200);
  Serial.println();
  Serial.printf("[boot] firmware built %s %s\n", __DATE__, __TIME__);
  // Include the exception number and faulting addresses retained across reset.
  // A reason-only message hides the information required to decode the crash.
  Serial.println(ESP.getResetInfo());
  storageReady = LittleFS.begin();
  configHealthy = loadConfig();
  if (!configHealthy) Serial.println(F("Cannot load config; schedules are empty."));
  WiFi.persistent(false);
  // Keep the local radio stable until an intentional router connection attempt.
  WiFi.mode(WIFI_AP);
  WiFi.setAutoReconnect(false); // Retries are managed by serviceConnectivity().
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  apClientJoined = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected&) {
    Serial.println(F("Local Wi-Fi: client joined"));
  });
  apClientLeft = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected&) {
    Serial.println(F("Local Wi-Fi: client disconnected"));
  });
  const IPAddress ip(192, 168, 4, 1);
  const bool configured = WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
  const bool started = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, 4);
  Serial.printf("AP: %s, configuration: %s, started: %s\n", AP_SSID,
                configured ? "OK" : "FAILED", started ? "OK" : "FAILED");
  Serial.print(F("Open http://"));
  Serial.println(WiFi.softAPIP());
  loadWifi();
  server.collectHeaders("Content-Type");
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.send_P(200, "text/html; charset=utf-8", WEB_UI);
  });
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/time", HTTP_POST, handleTime);
  server.on("/api/names", HTTP_POST, handleNames);
  server.on("/api/wifi", HTTP_POST, handleWifi);
  server.on("/api/relay", HTTP_POST, []() { handleRelay(false); });
  server.on("/api/auto", HTTP_POST, []() { handleRelay(true); });
  server.on("/api/schedule", HTTP_POST, []() { handleSchedule(false); });
  server.on("/api/schedule/delete", HTTP_POST, []() { handleSchedule(true); });
  server.onNotFound([]() { jsonError(404, "Không tìm thấy đường dẫn."); });
  server.begin();
}

void loop() {
  updateRelays();
  server.handleClient();
  serviceConnectivity();
  delay(2); // Let Wi-Fi and watchdog run.
}
