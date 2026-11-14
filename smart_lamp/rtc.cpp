#include "rtc.h"

#include <Arduino.h>
#include "config.h"

void rtc_init() {
  Serial.println("[rtc] init");
}

void rtc_update() {
  static bool logged = false;
  if (!logged) {
    logged = true;
    Serial.println("[rtc] update");
  }
}
