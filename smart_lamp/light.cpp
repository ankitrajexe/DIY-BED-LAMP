#include "light.h"

#include <Arduino.h>
#include "config.h"

void light_init() {
  Serial.println("[light] init");
}

void light_update() {
  static bool logged = false;
  if (!logged) {
    logged = true;
    Serial.println("[light] update");
  }
}
