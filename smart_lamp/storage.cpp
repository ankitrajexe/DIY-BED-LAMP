#include "storage.h"

#include <Arduino.h>
#include "config.h"

void storage_init() {
  Serial.println("[storage] init");
}

void storage_update() {
  static bool logged = false;
  if (!logged) {
    logged = true;
    Serial.println("[storage] update");
  }
}
