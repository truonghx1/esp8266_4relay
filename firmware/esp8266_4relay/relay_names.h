#pragma once
#include <stddef.h>
#include <stdint.h>

namespace relayapp {
// 24 Unicode code points, at most 96 UTF-8 bytes. Reject malformed UTF-8,
// embedded NULs and control characters before persisting user-entered names.
inline bool validRelayName(const char* text, size_t length) {
  if (!text || !length || length > 96) return false;
  size_t count = 0;
  bool nonSpace = false;
  for (size_t i = 0; i < length;) {
    uint32_t point = static_cast<uint8_t>(text[i++]);
    uint8_t extra = 0;
    uint32_t minimum = 0;
    if (point >= 0xC2 && point <= 0xDF) { point &= 0x1F; extra = 1; minimum = 0x80; }
    else if (point >= 0xE0 && point <= 0xEF) { point &= 0x0F; extra = 2; minimum = 0x800; }
    else if (point >= 0xF0 && point <= 0xF4) { point &= 7; extra = 3; minimum = 0x10000; }
    else if (point >= 0x80) return false;
    if (i + extra > length) return false;
    while (extra--) {
      const uint8_t next = static_cast<uint8_t>(text[i++]);
      if ((next & 0xC0) != 0x80) return false;
      point = (point << 6) | (next & 0x3F);
    }
    if (point < minimum || point > 0x10FFFF || (point >= 0xD800 && point <= 0xDFFF) ||
        point < 32 || (point >= 0x7F && point <= 0x9F) || point == 0x2028 || point == 0x2029)
      return false;
    if (++count > 24) return false;
    if (point != 32) nonSpace = true;
  }
  return nonSpace;
}
} // namespace relayapp
