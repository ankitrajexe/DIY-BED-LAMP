#ifndef RTC_H
#define RTC_H

/** Initialize the DS3231 RTC module. */
void rtc_init();

/** Periodic update for the DS3231 RTC module. */
void rtc_update();

#endif  // RTC_H
