#include "led.h"

#include <Arduino.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

namespace {

float s_warmLevel = 0.0f;
float s_coolLevel = 0.0f;

float clamp01(float value) {
  if (isnan(value) || value <= 0.0f) {
    return 0.0f;
  }
  if (value >= 1.0f) {
    return 1.0f;
  }
  return value;
}

uint32_t toPwmDuty(float level) {
  const float corrected = powf(clamp01(level), LED_GAMMA);
  const uint32_t maxDuty = (1UL << LED_PWM_RES_BITS) - 1UL;
  return static_cast<uint32_t>(lroundf(corrected * static_cast<float>(maxDuty)));
}

// --- Serial test helpers (remove in Step 13) ---
enum class SweepState : uint8_t {
  IDLE = 0,
  WARM,
  COOL
};

SweepState s_sweepState = SweepState::IDLE;
uint32_t s_sweepPhaseStartMs = 0;
uint32_t s_sweepLastStepMs = 0;

void cancelSweep(bool logStop) {
  if (s_sweepState != SweepState::IDLE) {
    s_sweepState = SweepState::IDLE;
    if (logStop) {
      Serial.println("[led] sweep stopped");
    }
  }
}

void startSweep() {
  s_sweepState = SweepState::WARM;
  s_sweepPhaseStartMs = millis();
  s_sweepLastStepMs = s_sweepPhaseStartMs;
  Serial.println("[led] sweep started");
  led_set(0.0f, 0.0f);
}

void updateSweep() {
  if (s_sweepState == SweepState::IDLE) {
    return;
  }

  const uint32_t now = millis();
  if ((now - s_sweepLastStepMs) < LED_SWEEP_STEP_MS) {
    return;
  }
  s_sweepLastStepMs = now;

  const uint32_t halfDurationMs =
      (LED_SWEEP_DURATION_MS > 1UL) ? (LED_SWEEP_DURATION_MS / 2UL) : 1UL;
  const uint32_t elapsed = now - s_sweepPhaseStartMs;

  if (s_sweepState == SweepState::WARM) {
    if (elapsed >= halfDurationMs) {
      led_set(1.0f, 0.0f);
      s_sweepState = SweepState::COOL;
      s_sweepPhaseStartMs = now;
      return;
    }
    const float level = static_cast<float>(elapsed) / static_cast<float>(halfDurationMs);
    led_set(level, 0.0f);
    return;
  }

  if (elapsed >= halfDurationMs) {
    led_set(0.0f, 1.0f);
    s_sweepState = SweepState::IDLE;
    Serial.println("[led] sweep done");
    return;
  }
  const float level = static_cast<float>(elapsed) / static_cast<float>(halfDurationMs);
  led_set(0.0f, level);
}

bool parseFloatArg(const char* str, float& out) {
  while (*str != '\0' && isspace(static_cast<unsigned char>(*str))) {
    ++str;
  }
  if (*str == '\0') {
    return false;
  }
  char* endPtr = nullptr;
  const float val = strtof(str, &endPtr);
  if (endPtr == str || !isfinite(val)) {
    return false;
  }
  while (*endPtr != '\0' && isspace(static_cast<unsigned char>(*endPtr))) {
    ++endPtr;
  }
  if (*endPtr != '\0') {
    return false;
  }
  out = val;
  return true;
}

void handleCommand(const char* rawCmd) {
  while (*rawCmd != '\0' && isspace(static_cast<unsigned char>(*rawCmd))) {
    ++rawCmd;
  }
  if (*rawCmd == '\0') {
    return;
  }

  char cmd[LED_CMD_BUF_SIZE];
  strncpy(cmd, rawCmd, sizeof(cmd) - 1);
  cmd[sizeof(cmd) - 1] = '\0';
  size_t len = strlen(cmd);
  while (len > 0 && isspace(static_cast<unsigned char>(cmd[len - 1]))) {
    cmd[--len] = '\0';
  }
  for (size_t i = 0; i < len; ++i) {
    cmd[i] = static_cast<char>(tolower(static_cast<unsigned char>(cmd[i])));
  }

  if (strcmp(cmd, "off") == 0) {
    cancelSweep(false);
    led_set(0.0f, 0.0f);
    return;
  }
  if (strcmp(cmd, "sweep") == 0) {
    startSweep();
    return;
  }
  if (strcmp(cmd, "stop") == 0) {
    if (s_sweepState == SweepState::IDLE) {
      Serial.println("[led] sweep not running");
    } else {
      cancelSweep(true);
    }
    return;
  }

  if ((cmd[0] == 'w' || cmd[0] == 'c' || cmd[0] == 'b') &&
      isspace(static_cast<unsigned char>(cmd[1]))) {
    float value = 0.0f;
    if (!parseFloatArg(cmd + 1, value)) {
      Serial.println("[led] invalid value");
      return;
    }
    cancelSweep(false);
    if (cmd[0] == 'w') {
      led_set(value, s_coolLevel);
    } else if (cmd[0] == 'c') {
      led_set(s_warmLevel, value);
    } else {
      led_set(value, value);
    }
    return;
  }

  Serial.println("[led] unknown cmd (use: w <v>, c <v>, b <v>, off, sweep, stop)");
}

void pollSerialCommands() {
  static char buf[LED_CMD_BUF_SIZE];
  static size_t pos = 0;
  static bool overflow = false;

  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\r' || ch == '\n') {
      if (overflow) {
        Serial.println("[led] cmd too long");
        overflow = false;
        pos = 0;
      } else if (pos > 0) {
        buf[pos] = '\0';
        pos = 0;
        handleCommand(buf);
      }
      continue;
    }
    if (overflow) {
      continue;
    }
    if (pos < (LED_CMD_BUF_SIZE - 1)) {
      buf[pos++] = ch;
    } else {
      overflow = true;
    }
  }
}

void ledTestUpdate() {
  pollSerialCommands();
  updateSweep();
}

}  // namespace

void led_init() {
  const bool warmOk = ledcAttach(LED_WARM_PIN, LED_PWM_FREQ_HZ, LED_PWM_RES_BITS);
  const bool coolOk = ledcAttach(LED_COOL_PIN, LED_PWM_FREQ_HZ, LED_PWM_RES_BITS);
  if (!warmOk || !coolOk) {
    Serial.printf("[led] error: ledcAttach failed (warm=%d cool=%d)\n", warmOk, coolOk);
  }

  s_warmLevel = 0.0f;
  s_coolLevel = 0.0f;
  ledcWrite(LED_WARM_PIN, 0);
  ledcWrite(LED_COOL_PIN, 0);

  Serial.printf("[led] init (freq=%luHz res=%u-bit gamma=%.1f)\n",
                static_cast<unsigned long>(LED_PWM_FREQ_HZ),
                static_cast<unsigned int>(LED_PWM_RES_BITS),
                static_cast<double>(LED_GAMMA));
  Serial.println("[led] cmd: w <0..1> | c <0..1> | b <0..1> | off | sweep | stop");
}

void led_set(float warm, float cool) {
  s_warmLevel = clamp01(warm);
  s_coolLevel = clamp01(cool);

  const uint32_t warmDuty = toPwmDuty(s_warmLevel);
  const uint32_t coolDuty = toPwmDuty(s_coolLevel);

  ledcWrite(LED_WARM_PIN, warmDuty);
  ledcWrite(LED_COOL_PIN, coolDuty);

  Serial.printf("[led] duty warm=%lu cool=%lu\n",
                static_cast<unsigned long>(warmDuty),
                static_cast<unsigned long>(coolDuty));
}

void led_update() {
  ledTestUpdate();
}
