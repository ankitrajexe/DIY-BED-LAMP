#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <stdint.h>

/**
 * Initialize the activity tracking module.
 */
void activity_init();

/**
 * Notify that user input activity occurred (e.g. touch pad contact, button press).
 * Resets the display sleep inactivity timer immediately.
 */
void activity_notify();

/**
 * Get the timestamp (in millis) of the most recent user activity.
 *
 * @return Milliseconds timestamp of last activity.
 */
uint32_t activity_last_ms();

#endif  // ACTIVITY_H
