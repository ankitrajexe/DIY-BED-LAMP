#include "display.h"

#include <Arduino.h>
#include "config.h"

void display_init() {
  Serial.println("[display] init");
}

void display_update() {
  static bool logged = false;
  if (!logged) {
    logged = true;
    Serial.println("[display] update");
  }
}
