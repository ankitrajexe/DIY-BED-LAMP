#include "fade.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "led.h"
#include "settings.h"

// In C++, wrapping variables in an unnamed namespace makes them private to this
// file, exactly like prefixing variables with an underscore in Python modules.
namespace {

// Current brightness of each channel (0.0 = completely off, 1.0 = full power).
float s_currentWarm = 0.0f;
float s_currentCool = 0.0f;

// Starting brightness for the active transition.
float s_startWarm = 0.0f;
float s_startCool = 0.0f;

// Target brightness we are ramping toward.
float s_targetWarm = 0.0f;
float s_targetCool = 0.0f;

// Timestamp (in milliseconds) when the current fade began.
uint32_t s_fadeStartMs = 0;

// Total requested duration of the current fade.
uint32_t s_fadeDurationMs = 0;

// Flag indicating whether a fade transition is currently running.
bool s_fadeActive = false;

// Timestamp of the last time we updated the LED hardware (for 50 Hz throttling).
uint32_t s_lastUpdateMs = 0;

/**
 * Helper function to keep a floating-point number strictly inside 0.0 to 1.0.
 * In Python, this is equivalent to max(0.0, min(1.0, value)).
 */
float clamp01(float value) {
  if (isnan(value) || value <= 0.0f) {
    return 0.0f;
  }
  if (value >= 1.0f) {
    return 1.0f;
  }
  return value;
}

}  // namespace

// ---------------------------------------------------------------------------
// Step 3 & Step 6 Serial test helper (remove in Step 19)
// ---------------------------------------------------------------------------

bool fade_handle_command(const char* cmd) {
  // Check for Step 6 "factory" reset command
  if (strcmp(cmd, "factory") == 0) {
    settings_factory_reset();
    return true;
  }

  // Check for Step 3 fade command: "f <warm> <cool> <ms>" (or "F ...")
  if ((cmd[0] == 'f' || cmd[0] == 'F') && (cmd[1] == ' ' || cmd[1] == '\t')) {
    float targetWarm = 0.0f;
    float targetCool = 0.0f;
    unsigned long duration = 0;

    const int matched = sscanf(cmd + 1, "%f %f %lu", &targetWarm, &targetCool, &duration);
    if (matched == 3) {
      fade_to(targetWarm, targetCool, static_cast<uint32_t>(duration));
    } else {
      Serial.println("[fade] usage: f <warm 0..1> <cool 0..1> <ms>");
    }
    return true;
  }

  return false;
}

void fade_init() {
  s_currentWarm = 0.0f;
  s_currentCool = 0.0f;
  s_startWarm = 0.0f;
  s_startCool = 0.0f;
  s_targetWarm = 0.0f;
  s_targetCool = 0.0f;
  s_fadeStartMs = 0;
  s_fadeDurationMs = 0;
  s_fadeActive = false;
  s_lastUpdateMs = 0;

  Serial.printf("[fade] init (rate=%luHz)\n",
                static_cast<unsigned long>(1000UL / FADE_INTERVAL_MS));
  Serial.println("[fade] cmd: f <warm> <cool> <ms>");
}

void fade_to(float targetWarm, float targetCool, uint32_t durationMs) {
  const float clampedWarm = clamp01(targetWarm);
  const float clampedCool = clamp01(targetCool);

  // If a fade is currently running, compute where we are RIGHT NOW and use that
  // as the new starting point. This prevents brightness from visibly jumping!
  if (s_fadeActive && s_fadeDurationMs > 0) {
    const uint32_t elapsed = millis() - s_fadeStartMs;
    if (elapsed < s_fadeDurationMs) {
      const float progress = static_cast<float>(elapsed) / static_cast<float>(s_fadeDurationMs);
      s_currentWarm = s_startWarm + (s_targetWarm - s_startWarm) * progress;
      s_currentCool = s_startCool + (s_targetCool - s_startCool) * progress;
    } else {
      s_currentWarm = s_targetWarm;
      s_currentCool = s_targetCool;
    }
  }

  s_startWarm = s_currentWarm;
  s_startCool = s_currentCool;
  s_targetWarm = clampedWarm;
  s_targetCool = clampedCool;
  s_fadeDurationMs = durationMs;
  s_fadeStartMs = millis();

  // If duration is 0, jump immediately to the target without waiting
  if (durationMs == 0) {
    s_currentWarm = clampedWarm;
    s_currentCool = clampedCool;
    s_fadeActive = false;
    led_set(s_currentWarm, s_currentCool);
    Serial.printf("[fade] done: target warm=%.2f cool=%.2f elapsed=0 ms\n",
                  static_cast<double>(s_currentWarm),
                  static_cast<double>(s_currentCool));
    return;
  }

  s_fadeActive = true;
  Serial.printf("[fade] start: (%.2f, %.2f) -> (%.2f, %.2f) over %lu ms\n",
                static_cast<double>(s_startWarm),
                static_cast<double>(s_startCool),
                static_cast<double>(s_targetWarm),
                static_cast<double>(s_targetCool),
                static_cast<unsigned long>(durationMs));
}

void fade_update() {
  // If no transition is running, nothing else to do
  if (!s_fadeActive) {
    return;
  }

  // Throttle updates to ~50 Hz (every FADE_INTERVAL_MS = 20 ms)
  const uint32_t now = millis();
  if ((now - s_lastUpdateMs) < FADE_INTERVAL_MS) {
    return;
  }
  s_lastUpdateMs = now;

  const uint32_t elapsed = now - s_fadeStartMs;

  // Check if the fade has reached its full duration
  if (elapsed >= s_fadeDurationMs) {
    s_currentWarm = s_targetWarm;
    s_currentCool = s_targetCool;
    s_fadeActive = false;
    led_set(s_currentWarm, s_currentCool);
    Serial.printf("[fade] done: target warm=%.2f cool=%.2f elapsed=%lu ms\n",
                  static_cast<double>(s_targetWarm),
                  static_cast<double>(s_targetCool),
                  static_cast<unsigned long>(elapsed));
    return;
  }

  // Linear interpolation: progress goes smoothly from 0.0 to 1.0
  const float progress = static_cast<float>(elapsed) / static_cast<float>(s_fadeDurationMs);
  s_currentWarm = s_startWarm + (s_targetWarm - s_startWarm) * progress;
  s_currentCool = s_startCool + (s_targetCool - s_startCool) * progress;

  led_set(s_currentWarm, s_currentCool);
}

bool fade_is_active() {
  return s_fadeActive;
}

float fade_get_warm() {
  return s_currentWarm;
}

float fade_get_cool() {
  return s_currentCool;
}
