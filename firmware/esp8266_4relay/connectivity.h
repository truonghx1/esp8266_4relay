#pragma once
#include "firebase_body.h"
#include "firebase_relays.h"
#include "wifi_credentials.h"
#include <memory>
#include <new>

bool wifiStartPending = false;
uint32_t wifiStartQueuedAt = 0;
String lastFirebaseReport;
uint32_t lastFirebaseReportAt = 0;
bool firebaseReportPending = false;
uint8_t firebaseMissingChannel = 0;
// Only trivial/empty initialization here. Never parse certificates inside a
// function-local static: ESP8266's __cxa_guard_acquire disables interrupts.
std::unique_ptr<BearSSL::X509List> firebaseRoots;
BearSSL::Session firebaseSession;

void connectivityTrace(const char* stage) {
  Serial.printf("[net] %s; heap=%u; maxblock=%u; stack=%u\n", stage,
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(), ESP.getFreeContStack());
}

bool prepareFirebaseTls() {
  if (firebaseRoots) return true;
  connectivityTrace("TLS roots parse begin");
  yield();
  std::unique_ptr<BearSSL::X509List> candidate(new (std::nothrow) BearSSL::X509List());
  if (!candidate || !candidate->append(FIREBASE_CA)) {
    firebaseStatus = "Không đủ bộ nhớ để đọc chứng chỉ TLS";
    connectivityTrace("TLS roots parse failed");
    return false;
  }
  firebaseRoots = std::move(candidate);
  yield();
  connectivityTrace("TLS roots ready");
  return true;
}

// Included after the existing local API helpers; schedules/config keep their format.
void beginWifi() {
  connectivityTrace("Wi-Fi begin");
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  connectivityTrace("Wi-Fi begin returned");
  wifiAttemptAt = millis();
  wifiConnecting = true;
  wifiWasConnected = false;
  cloudDirty = 0;
  wifiStatus = "Đang kết nối (tối đa 20 giây)";
  firebaseStatus = "Đang chờ Wi-Fi";
}

void loadWifi() {
  if (!storageReady || !LittleFS.exists("/wifi.json")) return;
  File file = LittleFS.open("/wifi.json", "r");
  StaticJsonDocument<384> doc;
  if (!file || file.size() > WIFI_FILE_LIMIT || deserializeJson(doc, file)) return;
  if (!validWifiCredentials(doc["ssid"], doc["password"])) return;
  wifiSsid = doc["ssid"].as<String>();
  wifiPassword = doc["password"].as<String>();
  if (wifiSsid.length() && wifiSsid.length() <= 32 && wifiPassword.length() <= 63) {
    // Leave a stable AP-only window after boot, including with stale credentials.
    wifiAttemptAt = millis();
    wifiStatus = "Chờ kết nối Wi-Fi nhà; ưu tiên truy cập local";
  } else {
    wifiSsid = ""; wifiPassword = "";
  }
}

void handleWifi() {
  // HTTP parsing/server frames already occupy the ESP8266's small task stack.
  DynamicJsonDocument doc(384);
  if (!readBody(doc)) return;
  if (!doc["ssid"].is<String>() || !doc["password"].is<String>()) {
    jsonError(400, "Nhập tên Wi-Fi và mật khẩu."); return;
  }
  const String ssid = doc["ssid"].as<String>(), password = doc["password"].as<String>();
  if (!validWifiCredentials(doc["ssid"], doc["password"])) {
    jsonError(400, "SSID 1–32 byte; mật khẩu 8–63 byte, hoặc để trống cho mạng mở."); return;
  }
  if (!storageReady) { jsonError(507, "Không lưu được cấu hình Wi-Fi."); return; }
  // Store only credentials, never echo the password through /api/state.
  DynamicJsonDocument saved(384);
  saved["ssid"] = ssid; saved["password"] = password;
  if (saved.overflowed()) { jsonError(503, "Không đủ bộ nhớ để lưu Wi-Fi."); return; }
  connectivityTrace("Saving Wi-Fi");
  File file = LittleFS.open("/wifi.tmp", "w");
  if (!file) { jsonError(507, "Không lưu được Wi-Fi."); return; }
  const size_t written = serializeJson(saved, file);
  file.flush();
  const bool complete = written == measureJson(saved) && file.size() == written;
  file.close();
  if (!complete || !LittleFS.rename("/wifi.tmp", "/wifi.json")) {
    LittleFS.remove("/wifi.tmp"); jsonError(507, "Không lưu được Wi-Fi."); return;
  }
  wifiSsid = ssid; wifiPassword = password;
  connectivityTrace("Wi-Fi saved");
  // Finish this HTTP handler and release its stack before touching the radio.
  wifiStartPending = true;
  wifiStartQueuedAt = millis();
  wifiStatus = "Đang kết nối";
  sendState();
}

int firebaseRequest(const String& path, const char* method, const String& body,
                    String& response, bool createOnly = false) {
  if (!firebaseRoots) return HTTPC_ERROR_TOO_LESS_RAM;
  // Leave room for the default 16 KiB TLS receive buffer, crypto state, TCP,
  // and the AP/STA SDK. The SDK can crash rather than recover from OOM.
  if (ESP.getFreeHeap() < 32000 || ESP.getMaxFreeBlockSize() < 18000) {
    connectivityTrace("TLS deferred: low heap");
    return HTTPC_ERROR_TOO_LESS_RAM;
  }
  // Trust anchors must outlive the TLS client, including its destructor.
  BearSSL::WiFiClientSecure client;
  client.setTrustAnchors(firebaseRoots.get());
  client.setSession(&firebaseSession);
  client.setTimeout(2500);
  HTTPClient http;
  String url = String(DATABASE_URL) + DEVICE_PATH + path + ".json";
  if (strlen(FIREBASE_AUTH_TOKEN)) url += "?auth=" + String(FIREBASE_AUTH_TOKEN);
  http.setTimeout(2500);
  http.setReuse(false);
  if (!http.begin(client, url)) return -1;
  http.addHeader("Content-Type", "application/json");
  if (createOnly) http.addHeader("if-match", "null_etag");
  connectivityTrace("Firebase HTTPS request");
  yield();
  int code = http.sendRequest(method, body);
  // Only consume our bounded schema; avoid allocating an arbitrary remote body.
  FirebaseBody received;
  response = "";
  if (code > 0 && http.getSize() <= 4096) {
    const int written = http.writeToStream(&received);
    const bool unboundedFull = http.getSize() < 0 && received.body.length() == FirebaseBody::LIMIT;
    if (written < 0 || received.overflow || received.allocationFailed || unboundedFull) {
      // A 200 header alone is not a successful read of the response body.
      Serial.printf("[net] Firebase body read failed: %d\n", written);
      if (code >= 200 && code < 300)
        code = written < 0 ? written : HTTPC_ERROR_STREAM_WRITE;
    } else response = received.body;
  } else if (code >= 200 && code < 300) code = HTTPC_ERROR_STREAM_WRITE;
  http.end();
  Serial.printf("[net] Firebase %s %s: HTTP/result=%d, bytes=%u\n",
                method, path.c_str(), code, response.length());
  yield();
  return code;
}

bool firebaseOK(int code) {
  if (code >= 200 && code < 300) return true;
  if (code == HTTPC_ERROR_TOO_LESS_RAM) {
    firebaseStatus = "Tạm hoãn Firebase: thiếu RAM cho TLS; local vẫn hoạt động";
    return false;
  }
  firebaseStatus = (code == 401 || code == 403) ? "Firebase từ chối quyền truy cập" :
                   "Lỗi Firebase/Internet: " + String(code);
  return false;
}

// JSON storage is destroyed before the caller starts a TLS handshake.
bool buildFirebaseReport(String& body, String& signature) {
  DynamicJsonDocument report(1024);
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    report["reported"][String(i + 1)] = (relayLogic.output() >> i) & 1U;
    report["names"][String(i + 1)] = relayNames[i];
  }
  if (report.overflowed()) return false;
  serializeJson(report, signature);
  if (signature.length() != measureJson(report)) return false;
  report["lastSeen"][".sv"] = "timestamp";
  if (report.overflowed()) return false;
  serializeJson(report, body);
  return body.length() == measureJson(report);
}

void syncFirebase() {
  String response, body;
  if (cloudDirty) {
    firebaseReportPending = false;
    {
      DynamicJsonDocument changes(192);
      for (uint8_t i = 0; i < RELAY_COUNT; ++i)
        if (cloudDirty & (1U << i)) changes[String(i + 1)] = (relayLogic.output() >> i) & 1U;
      if (changes.overflowed()) { firebaseStatus = "Không đủ bộ nhớ Firebase"; return; }
      serializeJson(changes, body);
      if (body.length() != measureJson(changes)) {
        firebaseStatus = "Không đủ bộ nhớ gửi thay đổi Firebase"; return;
      }
    } // Release JSON before connecting.
    if (!firebaseOK(firebaseRequest("/relays", "PATCH", body, response))) return;
    cloudDirty = 0;
    firebaseStatus = "Đã gửi thay đổi local";
    return;
  }
  if (firebaseMissingChannel) {
    const uint8_t id = firebaseMissingChannel;
    // Re-read after every attempt, even if creation is denied or the network
    // fails. Retrying this write forever would starve all incoming commands.
    firebaseMissingChannel = 0;
    int code = firebaseRequest("/relays/" + String(id), "PUT",
                               String((relayLogic.output() >> (id - 1)) & 1U), response, true);
    if (code != 412) firebaseOK(code);
    return;
  }
  if (firebaseReportPending) {
    // A denied/failed report must not prevent subsequent command reads.
    firebaseReportPending = false;
    String signature;
    if (!buildFirebaseReport(body, signature)) {
      firebaseStatus = "Không đủ bộ nhớ tạo báo cáo Firebase"; return;
    }
    if (signature == lastFirebaseReport && millis() - lastFirebaseReportAt < 10000) {
      firebaseReportPending = false;
      return;
    }
    if (firebaseOK(firebaseRequest("", "PATCH", body, response))) {
      lastFirebaseReport = signature;
      lastFirebaseReportAt = millis();
      firebaseReportPending = false;
      firebaseStatus = "Đã đồng bộ Firebase";
    }
    return;
  }
  if (!firebaseOK(firebaseRequest("/relays", "GET", "", response))) return;
  // A TLS request can cross a schedule boundary while the main loop is
  // blocked. Publish that newer local state before applying this older read.
  updateRelays();
  if (cloudDirty) return;
  uint8_t desired = 0, missing = 0;
  {
    DynamicJsonDocument doc(512);
    if (response.length() == 0 || deserializeJson(doc, response) ||
        !readFirebaseRelays(doc.as<JsonVariantConst>(), desired, missing)) {
      firebaseStatus = "Dữ liệu Firebase không hợp lệ"; return;
    }
  }
  // Firebase may encode numeric keys as an array (index zero is unused).
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    if (missing & (1U << i)) {
      // Conditional create cannot overwrite a simultaneous command from the web.
      firebaseMissingChannel = i + 1;
      firebaseStatus = "Đang khởi tạo kênh";
      break;
    }
  }
  cloudApplying = true;
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    if (missing & (1U << i)) continue;
    const int value = (desired >> i) & 1U;
    if (value != static_cast<int>((relayLogic.output() >> i) & 1U)) {
      relayLogic.setManual(i, value == 1);
      Serial.printf("[net] Applied web relay %u = %d\n", i + 1, value);
    }
  }
  updateRelays();
  cloudApplying = false;
  // Never open a second TLS connection while this GET's buffers are alive.
  String signature;
  if (!buildFirebaseReport(body, signature)) {
    firebaseStatus = "Không đủ bộ nhớ tạo báo cáo Firebase"; return;
  }
  firebaseReportPending = signature != lastFirebaseReport || millis() - lastFirebaseReportAt >= 10000;
  firebaseStatus = "Đã đọc lệnh Firebase";
}

void serviceConnectivity() {
  const uint32_t now = millis();
  if (wifiStartPending) {
    if (now - wifiStartQueuedAt < 500) return;
    wifiStartPending = false;
    beginWifi();
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      wifiWasConnected = false; cloudDirty = 0; wifiAttemptAt = now;
      WiFi.disconnect(true); // Disable STA only; keep the local AP available.
      wifiStatus = "Mất Wi-Fi; sẽ tự kết nối lại sau 30 giây";
      connectivityTrace("Router disconnected; retry in 30 s");
    }
    firebaseStatus = "Chưa kết nối Wi-Fi — chỉ dùng local";
    if (wifiConnecting && now - wifiAttemptAt >= 20000) {
      wifiConnecting = false; WiFi.disconnect(true);
      wifiStatus = "Không kết nối được; kiểm tra SSID/mật khẩu. Sẽ thử lại.";
      connectivityTrace("Wi-Fi attempt timed out; retry pending");
    } else if (!wifiConnecting && wifiSsid.length()) {
      // AP clients must not reset this timer: phones often stay associated
      // indefinitely, which otherwise prevents all automatic STA reconnects.
      if (now - wifiAttemptAt >= 30000) beginWifi();
    }
    return;
  }
  if (!wifiWasConnected) {
    wifiWasConnected = true; wifiConnecting = false; cloudDirty = 0;
    wifiStatus = "Đã kết nối";
    lastFirebaseReport = ""; // Force a fresh acknowledgement after reconnect.
    firebaseReportPending = false;
    firebaseMissingChannel = 0;
    connectivityTrace("Router connected; starting NTP");
    configTime(0, 0, "time.google.com", "pool.ntp.org");
    cloudAttemptAt = now;
    return; // Serve local requests before starting HTTPS in a later loop.
  }
  // Certificate verification needs wall time; never disable TLS verification.
  const time_t epoch = time(nullptr);
  if (epoch < 1704067200) { firebaseStatus = "Đang chờ giờ Internet để xác minh TLS"; return; }
  if (!deviceClock.valid()) {
    deviceClock.sync(static_cast<uint64_t>(epoch), now);
    // Initial clock recovery must not overwrite saved cloud commands before
    // their first read. Later schedule transitions remain normal local writes.
    cloudApplying = true;
    updateRelays(true);
    cloudApplying = false;
  }
  if (now - cloudAttemptAt < FIREBASE_POLL_MS) return;
  if (!firebaseRoots) {
    // Parse outside HTTP/JSON call frames, with interrupts enabled. Always give
    // the local server a separate loop iteration before opening a TLS socket.
    prepareFirebaseTls();
    cloudAttemptAt = millis();
    return;
  }
  syncFirebase();
  cloudAttemptAt = millis();
}
