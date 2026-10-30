#include <cassert>
#include <cstdint>
#include <string>
#include <iostream>
constexpr int WL_CONNECTED = 3;
struct {
  int connection = 0, clients = 2, disconnects = 0;
  int status() { return connection; }
  int softAPgetStationNum() { return clients; }
  void disconnect(bool) { ++disconnects; connection = 0; }
} WiFi;
uint32_t nowMs = 0, wifiAttemptAt = 0, wifiStartQueuedAt = 0;
bool wifiWasConnected = false, wifiConnecting = false, wifiStartPending = false;
uint8_t cloudDirty = 0;
std::string wifiSsid = "Saved network", wifiStatus, firebaseStatus;
int attempts = 0;
uint32_t millis() { return nowMs; }
void connectivityTrace(const char*) {}
void beginWifi() { ++attempts; wifiConnecting = true; wifiWasConnected = false; wifiAttemptAt = nowMs; }
// Actual offline/reconnect branch extracted from firmware by the runner.
#include "../build/wifi_reconnect_under_test.h"
int main() {
  // Boot with saved credentials and phones continuously associated to local AP.
  for (nowMs = 0; nowMs < 30000; ++nowMs) serviceConnectivity();
  assert(attempts == 0);
  serviceConnectivity(); assert(attempts == 1 && wifiConnecting);
  nowMs = 49999; serviceConnectivity(); assert(wifiConnecting);
  nowMs = 50000; serviceConnectivity(); assert(!wifiConnecting && WiFi.disconnects == 1);
  nowMs = 59999; serviceConnectivity(); assert(attempts == 1);
  nowMs = 60000; serviceConnectivity(); assert(attempts == 2);
  // Previously connected router drops; AP clients must not postpone the retry.
  wifiWasConnected = true; wifiConnecting = false; cloudDirty = 15;
  nowMs = 100000; serviceConnectivity();
  assert(!wifiWasConnected && cloudDirty == 0 && wifiAttemptAt == 100000);
  for (nowMs = 100001; nowMs < 130000; ++nowMs) serviceConnectivity();
  assert(attempts == 2);
  serviceConnectivity(); assert(attempts == 3);
  // No stored network: no attempts, even long after boot.
  wifiSsid.clear(); wifiConnecting = false;
  nowMs = 200000; serviceConnectivity(); assert(attempts == 3);
  // Explicit save bypasses the 30-second wait, but defers the radio by 500 ms.
  wifiSsid = "New network"; wifiStartPending = true; wifiStartQueuedAt = nowMs;
  nowMs += 499; serviceConnectivity(); assert(attempts == 3);
  ++nowMs; serviceConnectivity(); assert(attempts == 4 && !wifiStartPending);
  // Unsigned timer subtraction also works across millis() rollover.
  wifiConnecting = false; wifiAttemptAt = UINT32_MAX - 10000;
  nowMs = wifiAttemptAt + uint32_t(29999); serviceConnectivity(); assert(attempts == 4);
  ++nowMs; serviceConnectivity(); assert(attempts == 5);
  std::cout << "PASS: saved Wi-Fi boot, retry with AP clients, timeout, lost router, no credentials, explicit save and timer rollover\n";
}
