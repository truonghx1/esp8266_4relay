#pragma once
#include <Arduino.h>

// Wi-Fi riêng; mật khẩu phải có 8–63 ký tự. Đổi trước khi sử dụng.
constexpr char AP_SSID[] = "ESP8266-4Relay";
constexpr char AP_PASSWORD[] = "12345678";
constexpr uint8_t AP_CHANNEL = 6;

constexpr char DATABASE_URL[] = "https://iot-thx-default-rtdb.asia-southeast1.firebasedatabase.app";
constexpr char DEVICE_PATH[] = "/devices/esp8266_4relay";
// Optional Firebase ID token (expires); empty requires database rules allowing access.
// Never commit a real token. Browser credentials are configured separately.
constexpr char FIREBASE_AUTH_TOKEN[] = "";
constexpr uint32_t FIREBASE_POLL_MS = 2000;

// Relay 1..4: D1 (GPIO5), D2 (GPIO4), D5 (GPIO14), D6 (GPIO12).
constexpr uint8_t RELAY_PINS[4] = {D1, D2, D5, D6};
constexpr bool RELAY_ACTIVE_LOW = false;
constexpr int DEFAULT_UTC_OFFSET_MINUTES = 420;  // Việt Nam, UTC+7.

static_assert(sizeof(AP_PASSWORD) >= 9 && sizeof(AP_PASSWORD) <= 64,
              "AP password must contain 8..63 characters");
static_assert(sizeof(AP_SSID) > 1 && sizeof(AP_SSID) <= 33,
              "AP SSID must contain 1..32 characters");
