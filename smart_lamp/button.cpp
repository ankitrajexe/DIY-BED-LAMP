#include "button.h"

#include <Arduino.h>
#include "config.h"

void button_init() {
  Serial.println("[button] init");
}

void button_update() {
  static bool logged = false;
  if (!logged) {
    logged = true;
    Serial.println("[button] update");
  }
}
