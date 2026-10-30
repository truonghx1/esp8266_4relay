#pragma once

class FirebaseBody : public Stream {
 public:
  String body;
  bool overflow = false;
  size_t write(uint8_t c) override {
    if (body.length() >= 4096) { overflow = true; return 0; }
    body += static_cast<char>(c); return 1;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
};

// Included after the existing local API helpers; schedules/config keep their format.
void beginWifi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
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
  if (!file || file.size() > 384 || deserializeJson(doc, file)) return;
  if (!doc["ssid"].is<String>() || !doc["password"].is<String>()) return;
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
  StaticJsonDocument<384> doc;
  if (!readBody(doc)) return;
  if (!doc["ssid"].is<String>() || !doc["password"].is<String>()) {
    jsonError(400, "Nhập tên Wi-Fi và mật khẩu."); return;
  }
  const String ssid = doc["ssid"].as<String>(), password = doc["password"].as<String>();
  if (ssid.length() < 1 || ssid.length() > 32 || password.length() > 63 ||
      (password.length() > 0 && password.length() < 8) ||
      strlen(doc["ssid"].as<const char*>()) != doc["ssid"].as<JsonString>().size() ||
      strlen(doc["password"].as<const char*>()) != doc["password"].as<JsonString>().size()) {
    jsonError(400, "SSID 1–32 byte; mật khẩu 8–63 byte, hoặc để trống cho mạng mở."); return;
  }
  if (!storageReady) { jsonError(507, "Không lưu được cấu hình Wi-Fi."); return; }
  // Store only credentials, never echo the password through /api/state.
  StaticJsonDocument<384> saved;
  saved["ssid"] = ssid; saved["password"] = password;
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
  // Reply before moving the AP radio to the router's channel.
  wifiStatus = "Đang kết nối";
  sendState();
  beginWifi();
}

int firebaseRequest(const String& path, const char* method, const String& body,
                    String& response, bool createOnly = false) {
  BearSSL::WiFiClientSecure client;
  BearSSL::X509List roots(FIREBASE_CA);
  client.setTrustAnchors(&roots);
  client.setTimeout(2500);
  HTTPClient http;
  String url = String(DATABASE_URL) + DEVICE_PATH + path + ".json";
  if (strlen(FIREBASE_AUTH_TOKEN)) url += "?auth=" + String(FIREBASE_AUTH_TOKEN);
  http.setTimeout(2500);
  http.setReuse(false);
  if (!http.begin(client, url)) return -1;
  http.addHeader("Content-Type", "application/json");
  if (createOnly) http.addHeader("if-match", "null_etag");
  const int code = http.sendRequest(method, body);
  // Only consume our bounded schema; avoid allocating an arbitrary remote body.
  FirebaseBody received;
  if (code > 0 && http.getSize() <= 4096) {
    const int written = http.writeToStream(&received);
    response = written >= 0 && !received.overflow ? received.body : "";
  } else response = "";
  http.end();
  return code;
}

bool firebaseOK(int code) {
  if (code >= 200 && code < 300) return true;
  firebaseStatus = (code == 401 || code == 403) ? "Firebase từ chối quyền truy cập" :
                   "Lỗi Firebase/Internet: " + String(code);
  return false;
}

void syncFirebase() {
  String response, body;
  if (cloudDirty) {
    StaticJsonDocument<192> changes;
    for (uint8_t i = 0; i < RELAY_COUNT; ++i)
      if (cloudDirty & (1U << i)) changes[String(i + 1)] = (relayLogic.output() >> i) & 1U;
    serializeJson(changes, body);
    if (!firebaseOK(firebaseRequest("/relays", "PATCH", body, response))) return;
    cloudDirty = 0;
    firebaseStatus = "Đã gửi thay đổi local";
    return;
  }
  if (!firebaseOK(firebaseRequest("/relays", "GET", "", response))) return;
  StaticJsonDocument<512> doc;
  if (response.length() == 0 || deserializeJson(doc, response) ||
      !(doc.isNull() || doc.is<JsonObject>() || doc.is<JsonArray>())) {
    firebaseStatus = "Dữ liệu Firebase không hợp lệ"; return;
  }
  // Firebase may encode numeric keys as an array (index zero is unused).
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    JsonVariantConst value = doc.is<JsonArray>() ? doc[i + 1].as<JsonVariantConst>() : doc[String(i + 1)].as<JsonVariantConst>();
    if (value.isNull()) {
      // Conditional create cannot overwrite a simultaneous command from the web.
      int code = firebaseRequest("/relays/" + String(i + 1), "PUT",
                                 String((relayLogic.output() >> i) & 1U), response, true);
      if (code != 412 && !firebaseOK(code)) return;
      firebaseStatus = "Đang khởi tạo kênh";
      return;
    }
    if (!integerIn(value, 0, 1)) { firebaseStatus = "Kênh Firebase phải là số 0 hoặc 1"; return; }
  }
  cloudApplying = true;
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    const int value = doc.is<JsonArray>() ? doc[i + 1].as<int>() : doc[String(i + 1)].as<int>();
    if (value != ((relayLogic.output() >> i) & 1U)) relayLogic.setManual(i, value == 1);
  }
  updateRelays();
  cloudApplying = false;
  StaticJsonDocument<1024> report;
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    report["reported"][String(i + 1)] = (relayLogic.output() >> i) & 1U;
    report["names"][String(i + 1)] = relayNames[i];
  }
  report["lastSeen"][".sv"] = "timestamp";
  serializeJson(report, body);
  if (firebaseOK(firebaseRequest("", "PATCH", body, response))) firebaseStatus = "Đã đồng bộ Firebase";
}

void serviceConnectivity() {
  const uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      wifiWasConnected = false; cloudDirty = 0; wifiAttemptAt = now;
      WiFi.disconnect(true); // Disable STA only; keep the local AP available.
    }
    firebaseStatus = "Chưa kết nối Wi-Fi — chỉ dùng local";
    if (wifiConnecting && now - wifiAttemptAt >= 20000) {
      wifiConnecting = false; WiFi.disconnect(true);
      wifiStatus = "Không kết nối được; kiểm tra SSID/mật khẩu. Sẽ thử lại.";
    } else if (!wifiConnecting && wifiSsid.length()) {
      // Do not scan/change channels underneath a phone using the local AP.
      // Explicit submission of /api/wifi still connects immediately.
      if (WiFi.softAPgetStationNum() > 0) {
        wifiAttemptAt = now;
        wifiStatus = "Đang dùng local; kết nối Wi-Fi nhà khi lưu hoặc rời AP";
      } else if (now - wifiAttemptAt >= 30000) beginWifi();
      else if (wifiStatus == "Đã kết nối") wifiStatus = "Mất Wi-Fi; đang chờ thử lại";
    }
    return;
  }
  if (!wifiWasConnected) {
    wifiWasConnected = true; wifiConnecting = false; cloudDirty = 0;
    wifiStatus = "Đã kết nối";
    configTime(0, 0, "time.google.com", "pool.ntp.org");
    cloudAttemptAt = now - FIREBASE_POLL_MS;
  }
  // Certificate verification needs wall time; never disable TLS verification.
  const time_t epoch = time(nullptr);
  if (epoch < 1704067200) { firebaseStatus = "Đang chờ giờ Internet để xác minh TLS"; return; }
  if (!deviceClock.valid()) { deviceClock.sync(static_cast<uint64_t>(epoch), now); updateRelays(true); }
  if (now - cloudAttemptAt < FIREBASE_POLL_MS) return;
  syncFirebase();
  cloudAttemptAt = millis();
}
