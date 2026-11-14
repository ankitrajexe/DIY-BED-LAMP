#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>

/**
 * Touch event types emitted by the TTP223 touch sensor.
 * In Python, this is equivalent to an Enum:
 *   class TouchEvent(Enum):
 *       NONE = 0
 *       SINGLE = 1
 *       DOUBLE = 2
 *       TRIPLE = 3
 *       LONG = 4
 */
enum class TouchEvent : uint8_t {
  NONE = 0,
  SINGLE,
  DOUBLE,
  TRIPLE,
  LONG
};

/**
 * Initialize GPIO pin for TTP223 touch input (active HIGH).
 * If the pad is already held HIGH at boot, ignores it until it goes LOW once.
 */
void touch_init();

/**
 * Non-blocking state machine update, called every loop() iteration.
 * Handles debouncing, multi-tap window timing, and long-press detection.
 */
void touch_update();

/**
 * Poll for the latest touch event.
 * Returns the pending event and resets the internal event to NONE.
 *
 * @return Pending TouchEvent, or TouchEvent::NONE if no event is waiting.
 */
TouchEvent touch_get_event();

/**
 * Convert a TouchEvent enum into a human-readable string for logging.
 *
 * @param ev The TouchEvent to convert.
 * @return String representation ("SINGLE", "DOUBLE", "TRIPLE", "LONG", or "NONE").
 */
const char* touch_event_name(TouchEvent ev);

#endif  // TOUCH_H
