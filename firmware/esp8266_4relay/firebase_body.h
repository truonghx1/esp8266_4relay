#pragma once

// Stream/String are supplied by Arduino (or the host regression-test adapter).
// ESP8266 HTTPClient checks availableForWrite() before calling write().
class FirebaseBody : public Stream {
 public:
  static constexpr size_t LIMIT = 4096;
  String body;
  bool overflow = false;
  bool allocationFailed = false;

  int availableForWrite() override {
    return allocationFailed ? 0 : static_cast<int>(LIMIT - body.length());
  }
  bool outputCanTimeout() override { return false; }
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t* data, size_t size) override {
    if (size > LIMIT - body.length()) { overflow = true; return 0; }
    if (allocationFailed) return 0;
    if (!body.concat(reinterpret_cast<const char*>(data), size)) {
      allocationFailed = true; return 0;
    }
    return size;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
};
