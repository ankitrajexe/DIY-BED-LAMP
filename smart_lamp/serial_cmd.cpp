#include "serial_cmd.h"

#include <Arduino.h>
#include <ctype.h>
#include <string.h>
#include "fade.h"
#include "i2c_bus.h"
#include "led.h"
#include "rtc.h"

// ---------------------------------------------------------------------------
// Step 2-8 Isolated Serial test command dispatcher (remove in Step 19)
// ---------------------------------------------------------------------------

namespace {

// Maximum length for a serial command line
constexpr size_t SERIAL_CMD_BUF_SIZE = 64;

/**
 * Dispatch a single trimmed command line to each module in turn.
 * In Python, this is like:
 *   for handler in [led_handler, fade_handler, rtc_handler, i2c_handler]:
 *       if handler(line):
 *           return
 *   print("[cmd] unknown cmd")
 */
void dispatchCommand(char* line) {
  // Trim leading whitespace
  char* start = line;
  while (*start != '\0' && isspace(static_cast<unsigned char>(*start))) {
    ++start;
  }

  // Trim trailing whitespace and carriage returns
  char* end = start + strlen(start);
  while (end > start && isspace(static_cast<unsigned char>(*(end - 1)))) {
    --end;
  }
  *end = '\0';

  // If the line is empty (e.g. user just hit Enter), ignore silently
  if (*start == '\0') {
    return;
  }

  // Hand the command to each module's handler in turn
  if (led_handle_command(start)) {
    return;
  }
  if (fade_handle_command(start)) {
    return;
  }
  if (rtc_handle_command(start)) {
    return;
  }
  if (i2c_handle_command(start)) {
    return;
  }

  // Only if NONE of them recognised it, print one single unknown cmd message
  Serial.println("[cmd] unknown cmd");
}

}  // namespace

void serial_cmd_update() {
  static char buf[SERIAL_CMD_BUF_SIZE];
  static size_t pos = 0;
  static bool overflow = false;

  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());

    // When Enter is pressed (newline or carriage return), process line
    if (ch == '\r' || ch == '\n') {
      if (overflow) {
        Serial.println("[cmd] line too long");
        overflow = false;
        pos = 0;
      } else if (pos > 0) {
        buf[pos] = '\0';
        pos = 0;
        dispatchCommand(buf);
      }
      continue;
    }

    if (overflow) {
      continue;
    }

    // Accumulate characters into buffer until full
    if (pos < (SERIAL_CMD_BUF_SIZE - 1)) {
      buf[pos++] = ch;
    } else {
      overflow = true;
    }
  }
}
