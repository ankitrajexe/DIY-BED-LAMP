#include "activity.h"

#include <Arduino.h>

// In C++, the unnamed namespace keeps this state private to this file
namespace {
uint32_t s_lastActivityMs = 0;
}

void activity_init() {
  s_lastActivityMs = millis();
}

void activity_notify() {
  s_lastActivityMs = millis();
}

uint32_t activity_last_ms() {
  return s_lastActivityMs;
}
