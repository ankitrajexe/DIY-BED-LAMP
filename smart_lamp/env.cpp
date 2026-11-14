#include "env.h"

#include <Arduino.h>
#include "config.h"

void env_init() {
  Serial.println("[env] init");
}

void env_update() {
  static bool logged = false;
  if (!logged) {
    logged = true;
    Serial.println("[env] update");
  }
}
