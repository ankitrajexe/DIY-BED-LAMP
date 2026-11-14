#include "config.h"
#include "button.h"
#include "display.h"
#include "env.h"
#include "fade.h"
#include "lamp.h"
#include "led.h"
#include "light.h"
#include "rtc.h"
#include "storage.h"
#include "touch.h"

/**
 * Central initialization entry point called from setup().
 * Named app_init() because ESP32 Arduino core already declares extern "C" void init(void).
 */
void app_init() {
  Serial.begin(SERIAL_BAUD);
  if (SERIAL_WAIT_MS > 0) {
    const uint32_t serialStartMs = millis();
    while ((millis() - serialStartMs) < SERIAL_WAIT_MS) {
      yield();
    }
  }

  storage_init();
  led_init();
  fade_init();
  touch_init();
  button_init();
  light_init();
  rtc_init();
  env_init();
  display_init();
  lamp_init();
}

void setup() {
  app_init();
}

void loop() {
  touch_update();
  button_update();
  light_update();
  rtc_update();
  env_update();
  lamp_update();
  fade_update();
  display_update();
  storage_update();
  led_update();
}
