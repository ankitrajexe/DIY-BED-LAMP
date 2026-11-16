#include "rtc.h"

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "i2c_bus.h"
#include "settings.h"

// In C++, the unnamed namespace keeps variables private to this file
// (equivalent to module-level private variables in Python).
namespace {

// Adafruit RTClib hardware driver instance for DS3231
RTC_DS3231 s_rtc;

// Flag indicating whether DS3231 hardware was detected on I2C
bool s_rtcOk = false;

// Timestamp of the last periodic serial log output (for 10 s logging)
uint32_t s_lastLogMs = 0;

// ---------------------------------------------------------------------------
// Step 8 Serial test helper (remove in Step 19)
// ---------------------------------------------------------------------------

/**
 * Handle incoming test commands:
 *   "T YYYY-MM-DD HH:MM:SS" -> sets current time
 *   "i2c"                   -> scans I2C bus and prints all detected addresses
 */
void handleRtcTestCommand(const char* rawCmd) {
  // Trim leading whitespace
  const char* cmd = rawCmd;
  while (*cmd == ' ' || *cmd == '\t') {
    cmd++;
  }

  // Command: "i2c"
  if (strcasecmp(cmd, "i2c") == 0) {
    i2c_scan();
    return;
  }

  // Command: "T YYYY-MM-DD HH:MM:SS"
  if (cmd[0] == 'T' || cmd[0] == 't') {
    if (!s_rtcOk) {
      Serial.println("[rtc] cannot set time: RTC hardware not detected");
      return;
    }

    const char* args = cmd + 1;
    while (*args == ' ' || *args == '\t') {
      args++;
    }

    int y = 0, m = 0, d = 0, hr = 0, mn = 0, sc = 0;
    // sscanf parses text into integers, similar to Python's int() after split()
    const int matched = sscanf(args, "%d-%d-%d %d:%d:%d", &y, &m, &d, &hr, &mn, &sc);

    // Validate date and time ranges
    if (matched == 6 &&
        y >= 2000 && y <= 2099 &&
        m >= 1 && m <= 12 &&
        d >= 1 && d <= 31 &&
        hr >= 0 && hr <= 23 &&
        mn >= 0 && mn <= 59 &&
        sc >= 0 && sc <= 59) {
      s_rtc.adjust(DateTime(y, m, d, hr, mn, sc));
      Serial.printf("[rtc] time set: %04d-%02d-%02d %02d:%02d:%02d\n",
                    y, m, d, hr, mn, sc);
      // Immediately check if this manual adjustment triggered a date change
      rtc_date_changed();
    } else {
      Serial.println("[rtc] invalid format (use: T YYYY-MM-DD HH:MM:SS)");
    }
    return;
  }

  Serial.println("[rtc] unknown command (use: T YYYY-MM-DD HH:MM:SS | i2c)");
}

/**
 * Non-blocking serial listener for RTC and I2C commands.
 * Checks Serial.peek(). If the next character is 'T', 't', 'I', or 'i',
 * it reads and processes the line here. If not, it leaves the bytes in the
 * Serial buffer so other modules (fade, led) can handle them without conflicts!
 */
void pollRtcSerialTest() {
  static char buf[48];
  static size_t pos = 0;
  static bool readingCmd = false;

  while (Serial.available() > 0) {
    if (!readingCmd) {
      const char next = static_cast<char>(Serial.peek());
      if (next == 'T' || next == 't' || next == 'I' || next == 'i') {
        readingCmd = true;
      } else {
        // Not for us; leave in buffer for fade/led modules
        break;
      }
    }

    const char ch = static_cast<char>(Serial.read());
    if (ch == '\r' || ch == '\n') {
      if (pos > 0) {
        buf[pos] = '\0';
        pos = 0;
        readingCmd = false;
        handleRtcTestCommand(buf);
      } else {
        readingCmd = false;
      }
      continue;
    }

    if (pos < sizeof(buf) - 1) {
      buf[pos++] = ch;
    }
  }
}

}  // namespace

void rtc_init() {
  // 1. Probe I2C bus at DS3231 address (0x68)
  Wire.beginTransmission(DS3231_I2C_ADDR);
  const uint8_t err = Wire.endTransmission();
  if (err != 0) {
    s_rtcOk = false;
    Serial.printf("[rtc] DS3231 not found at 0x%02X (error=%d), skipping\n",
                  DS3231_I2C_ADDR, err);
    return;
  }

  // 2. Initialize Adafruit RTClib driver
  if (!s_rtc.begin()) {
    s_rtcOk = false;
    Serial.println("[rtc] s_rtc.begin() failed, skipping");
    return;
  }

  s_rtcOk = true;

  // 3. Power loss check: if battery died or board was unpowered with no CR2032,
  // set the clock to firmware build timestamp (__DATE__ and __TIME__)
  if (s_rtc.lostPower()) {
    s_rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("[rtc] lost power, time set from compile time (__DATE__ __TIME__)");
  }

  // Print startup timestamp
  DateTime now = s_rtc.now();
  Serial.printf("[rtc] init (addr=0x%02X) %04d-%02d-%02d %02d:%02d:%02d\n",
                DS3231_I2C_ADDR,
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());

  s_lastLogMs = millis();

  // 4. Check for date change on boot (Architecture Rule 3)
  rtc_date_changed();
}

void rtc_update() {
  // Always poll for test commands
  pollRtcSerialTest();

  // If RTC hardware was not detected, do nothing (fails gracefully)
  if (!s_rtcOk) {
    return;
  }

  const uint32_t nowMs = millis();

  // Check date change rollover during runtime
  rtc_date_changed();

  // Log timestamp periodically every RTC_LOG_INTERVAL_MS (10 s)
  if ((nowMs - s_lastLogMs) >= RTC_LOG_INTERVAL_MS) {
    s_lastLogMs = nowMs;
    DateTime now = s_rtc.now();
    Serial.printf("[rtc] %04d-%02d-%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());
  }
}

bool rtc_ok() {
  return s_rtcOk;
}

DateTime rtc_now() {
  if (s_rtcOk) {
    return s_rtc.now();
  }
  // Safe fallback if RTC is offline: compile-time snapshot
  return DateTime(F(__DATE__), F(__TIME__));
}

bool rtc_date_changed() {
  if (!s_rtcOk) {
    return false;
  }

  DateTime now = s_rtc.now();
  // Format as integer YYYYMMDD (e.g. 20261116) for clean comparison
  const uint32_t today = static_cast<uint32_t>(now.year()) * 10000UL +
                         static_cast<uint32_t>(now.month()) * 100UL +
                         static_cast<uint32_t>(now.day());

  Settings& cfg = settings_get_mutable();

  // First ever boot on fresh chip: initialize last_seen_date without triggering date change
  if (cfg.last_seen_date == 0) {
    cfg.last_seen_date = today;
    settings_mark_dirty();
    return false;
  }

  // Date changed while running or while lamp was powered off
  if (today != cfg.last_seen_date) {
    Serial.printf("[rtc] date changed: %lu -> %lu\n",
                  static_cast<unsigned long>(cfg.last_seen_date),
                  static_cast<unsigned long>(today));
    cfg.last_seen_date = today;
    settings_mark_dirty();
    return true;
  }

  return false;
}
