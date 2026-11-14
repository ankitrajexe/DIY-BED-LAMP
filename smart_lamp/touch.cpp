#include "touch.h"

#include <Arduino.h>
#include "config.h"

// In C++, the unnamed namespace keeps these variables private to this file
// (like module-level variables with a leading underscore in Python).
namespace {

// Debounced state of the touch pad (false = released, true = touched).
bool s_debouncedState = false;

// Last raw reading from the GPIO pin (for change detection).
bool s_lastRawReading = false;

// Timestamp of the last raw reading change (for debounce timing).
uint32_t s_lastDebounceMs = 0;

// Timestamp when the current continuous press began.
uint32_t s_pressStartMs = 0;

// Timestamp when the last tap was released (for tap-window timeout).
uint32_t s_lastReleaseMs = 0;

// Number of taps accumulated within the current tap window (0, 1, 2, or 3).
uint8_t s_tapCount = 0;

// Flag indicating whether the user is currently holding the pad down.
bool s_isHeld = false;

// Flag ensuring LONG event fires only once per continuous hold.
bool s_longPressFired = false;

// Startup guard: if the pad is touched during power-on/reset, ignore it
// until it is released at least once.
bool s_ignoreUntilRelease = false;

// The latest unconsumed event waiting for touch_get_event().
TouchEvent s_pendingEvent = TouchEvent::NONE;

/**
 * Helper to record and print an event.
 */
void emitEvent(TouchEvent ev) {
  s_pendingEvent = ev;
  Serial.printf("[touch] event %s\n", touch_event_name(ev));
}

}  // namespace

const char* touch_event_name(TouchEvent ev) {
  switch (ev) {
    case TouchEvent::SINGLE: return "SINGLE";
    case TouchEvent::DOUBLE: return "DOUBLE";
    case TouchEvent::TRIPLE: return "TRIPLE";
    case TouchEvent::LONG:   return "LONG";
    default:                 return "NONE";
  }
}

void touch_init() {
  // Configure GPIO3 as a digital input (TTP223 provides active HIGH output)
  pinMode(TOUCH_PIN, INPUT);

  const bool initialRaw = (digitalRead(TOUCH_PIN) == HIGH);
  s_debouncedState = initialRaw;
  s_lastRawReading = initialRaw;
  s_lastDebounceMs = millis();
  s_pressStartMs = 0;
  s_lastReleaseMs = 0;
  s_tapCount = 0;
  s_isHeld = false;
  s_longPressFired = false;
  s_pendingEvent = TouchEvent::NONE;

  // If pad is already HIGH on boot, ignore input until user releases it
  s_ignoreUntilRelease = initialRaw;

  Serial.println("[touch] init");
  if (s_ignoreUntilRelease) {
    Serial.println("[touch] pad HIGH at boot, ignoring until released");
  }
}

void touch_update() {
  const uint32_t now = millis();
  const bool rawReading = (digitalRead(TOUCH_PIN) == HIGH);

  // If we are waiting for release after boot, do nothing until pin is LOW
  if (s_ignoreUntilRelease) {
    if (!rawReading) {
      s_ignoreUntilRelease = false;
      s_debouncedState = false;
      s_lastRawReading = false;
      s_lastDebounceMs = now;
      Serial.println("[touch] pad released, input enabled");
    }
    return;
  }

  // 1. Debounce logic: check if raw GPIO pin reading has changed
  if (rawReading != s_lastRawReading) {
    s_lastRawReading = rawReading;
    s_lastDebounceMs = now;
  }

  // 2. Only accept the state change after it remains stable for TOUCH_DEBOUNCE_MS
  if ((now - s_lastDebounceMs) >= TOUCH_DEBOUNCE_MS) {
    if (rawReading != s_debouncedState) {
      s_debouncedState = rawReading;

      if (s_debouncedState) {
        // --- Pad PRESSED (LOW -> HIGH) ---
        s_isHeld = true;
        s_pressStartMs = now;
        s_longPressFired = false;
      } else {
        // --- Pad RELEASED (HIGH -> LOW) ---
        s_isHeld = false;

        // If this hold already fired a LONG press, do not count it as a tap
        if (!s_longPressFired) {
          s_tapCount++;
          s_lastReleaseMs = now;
        }
        s_longPressFired = false;
      }
    }
  }

  // 3. Long press detection while pad is continuously held
  if (s_isHeld && !s_longPressFired) {
    if ((now - s_pressStartMs) >= LONG_PRESS_MS) {
      s_longPressFired = true;
      s_tapCount = 0;  // Discard any preceding taps
      emitEvent(TouchEvent::LONG);
    }
  }

  // 4. Tap window timeout: decide SINGLE, DOUBLE, or TRIPLE after TAP_WINDOW_MS
  // only when pad is currently released (not held) and taps are waiting
  if (!s_isHeld && s_tapCount > 0) {
    if ((now - s_lastReleaseMs) >= TAP_WINDOW_MS) {
      if (s_tapCount == 1) {
        emitEvent(TouchEvent::SINGLE);
      } else if (s_tapCount == 2) {
        emitEvent(TouchEvent::DOUBLE);
      } else {
        emitEvent(TouchEvent::TRIPLE);
      }
      s_tapCount = 0;
    }
  }
}

TouchEvent touch_get_event() {
  const TouchEvent ev = s_pendingEvent;
  s_pendingEvent = TouchEvent::NONE;
  return ev;
}
