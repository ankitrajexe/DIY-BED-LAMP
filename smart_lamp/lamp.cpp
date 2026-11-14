#include "lamp.h"

#include <Arduino.h>
#include "config.h"

void lamp_init() {
  Serial.println("[lamp] init");
}

void lamp_update() {
  static bool logged = false;
  if (!logged) {
    logged = true;
    Serial.println("[lamp] update");
  }
}
