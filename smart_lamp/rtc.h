#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include <RTClib.h>

/**
 * Initialize the DS3231 RTC on the shared I2C bus (address 0x68).
 * Checks presence on I2C first. If not found, logs once and fails gracefully
 * so the lamp continues working normally (rtc_ok() returns false).
 * If power loss is detected, sets time from compile time (__DATE__ and __TIME__).
 */
void rtc_init();

/**
 * Periodic non-blocking update called every loop() iteration.
 * Handles periodic timestamp logging (every RTC_LOG_INTERVAL_MS = 10 s),
 * date-change checks, and serial test commands ("T" and "i2c").
 */
void rtc_update();

/**
 * Check if the DS3231 RTC hardware was detected and is operational.
 *
 * @return true if RTC responded at boot, false if offline / missing.
 */
bool rtc_ok();

/**
 * Get current date and time from the RTC.
 * If the RTC is offline, returns compile time as a fallback.
 *
 * @return DateTime object representing the current wall-clock time.
 */
DateTime rtc_now();

/**
 * Architecture Rule 3: Date change detection.
 * Compares current date against last_seen_date stored in Settings.
 * Returns true ONCE per date rollover (including at boot if date changed while off),
 * updates last_seen_date, and marks settings dirty so it gets saved to flash.
 * First ever boot with last_seen_date = 0 returns false without logging.
 *
 * @return true once on date change, false otherwise.
 */
bool rtc_date_changed();

#endif  // RTC_H
