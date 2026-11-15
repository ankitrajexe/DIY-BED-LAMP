#include "lamp.h"

#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "fade.h"
#include "settings.h"
#include "touch.h"

// In C++, the unnamed namespace keeps module state private to this file
// (equivalent to module-level private variables in Python).
namespace {

// Main state variables of the lamp
bool s_isOn = false;
uint8_t s_levelIndex = BRIGHTNESS_DEFAULT_LEVEL_INDEX;
LampPreset s_preset = LampPreset::NEUTRAL;
LampPreset s_prevPreset = LampPreset::NEUTRAL;  // Saved before entering night mode
bool s_nightMode = false;

/**
 * Helper to clamp a float to the [0.0, 1.0] range.
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

/**
 * Get human-readable name for a colour preset.
 */
const char* presetName(LampPreset p) {
  switch (p) {
    case LampPreset::WARM:    return "WARM";
    case LampPreset::NEUTRAL: return "NEUTRAL";
    case LampPreset::COOL:    return "COOL";
    default:                  return "UNKNOWN";
  }
}

/**
 * Look up the warm and cool mix ratios (0.0 to 1.0) for a given preset.
 */
ColorPresetMix getPresetMix(LampPreset p) {
  switch (p) {
    case LampPreset::WARM:    return PRESET_WARM;
    case LampPreset::COOL:    return PRESET_COOL;
    case LampPreset::NEUTRAL:
    default:                  return PRESET_NEUTRAL;
  }
}

/**
 * Print the current lamp state to Serial in the requested format:
 * "[lamp] ON level=2 (50%) preset=WARM night=0"
 */
void printState() {
  const uint8_t levelNumber = s_levelIndex + 1;
  const uint8_t percent = BRIGHTNESS_LEVELS[s_levelIndex];
  Serial.printf("[lamp] %s level=%u (%u%%) preset=%s night=%d\n",
                s_isOn ? "ON" : "OFF",
                levelNumber,
                percent,
                presetName(s_preset),
                s_nightMode ? 1 : 0);
}

/**
 * Architecture Rule 1: Central brightness computation pipeline.
 * Evaluates target levels strictly through ordered layers:
 *   (a) User brightness level and colour preset
 *   (b) Auto brightness from BH1750 (Step 11 placeholder)
 *   (c) Night factor (caps brightness at 35% and forces warm preset)
 *   (d) Pomodoro break dimming (Step 13 placeholder)
 *   (e) Sunrise alarm override (Step 17 placeholder)
 *
 * @param[out] outWarm Target brightness for warm LED channel (0.0 to 1.0).
 * @param[out] outCool Target brightness for cool LED channel (0.0 to 1.0).
 */
void lamp_compute_target(float& outWarm, float& outCool) {
  // If the lamp is switched off, output is strictly 0.0
  if (!s_isOn) {
    outWarm = 0.0f;
    outCool = 0.0f;
    return;
  }

  // --- Layer (a): User level and colour preset ---
  uint8_t userPercent = BRIGHTNESS_LEVELS[s_levelIndex];
  if (userPercent < MIN_BRIGHTNESS_PERCENT) {
    userPercent = MIN_BRIGHTNESS_PERCENT;
  }
  float brightness = static_cast<float>(userPercent) / 100.0f;
  ColorPresetMix mix = getPresetMix(s_preset);

  // --- Layer (b): Auto brightness from BH1750 (Step 11 stub) ---
  // Ambient light offset will be added here in Step 11. Pass-through for now.

  // --- Layer (c): Night factor ---
  if (s_nightMode) {
    const float nightCap = static_cast<float>(NIGHT_BRIGHTNESS_PERCENT) / 100.0f;
    if (brightness > nightCap) {
      brightness = nightCap;
    }
    // Night mode enforces warm colour temperature
    mix = PRESET_WARM;
  }

  // --- Layer (d): Pomodoro break dimming (Step 13 stub) ---
  // Break-time dimming will be applied here in Step 13. Pass-through for now.

  // --- Layer (e): Sunrise alarm override (Step 17 stub) ---
  // Sunrise wake-up ramp will override here in Step 17. Pass-through for now.

  // Final channel brightness computation
  outWarm = clamp01(brightness * mix.warm);
  outCool = clamp01(brightness * mix.cool);
}

/**
 * Helper to recompute target and initiate a fade.
 */
void applyTarget(uint32_t fadeDurationMs) {
  float targetWarm = 0.0f;
  float targetCool = 0.0f;
  lamp_compute_target(targetWarm, targetCool);
  fade_to(targetWarm, targetCool, fadeDurationMs);
}

/**
 * Synchronize current lamp state into Settings and mark flash dirty.
 */
void syncSettings() {
  Settings& cfg = settings_get_mutable();
  cfg.lamp_on = s_isOn;
  cfg.level_index = s_levelIndex;
  // If in night mode, preserve the user's base preset so exiting night mode restores it
  cfg.preset = s_nightMode ? s_prevPreset : s_preset;
  cfg.night_mode = s_nightMode;
  settings_mark_dirty();
}

}  // namespace

void lamp_init() {
  // Step 6: Load persistent settings from flash
  const Settings& cfg = settings_get();

  s_levelIndex = cfg.level_index;
  s_preset = cfg.preset;
  s_prevPreset = cfg.preset;
  s_nightMode = cfg.night_mode;

  // If night mode was stored, enforce the warm preset mix
  if (s_nightMode) {
    s_preset = LampPreset::WARM;
  }

  // Restore power state depending on config.h setting
  if (RESTORE_POWER_STATE != 0) {
    s_isOn = cfg.lamp_on;
  } else {
    s_isOn = false;
  }

  if (s_isOn) {
    // Restored lamp is ON at boot: fade up smoothly over BOOT_FADE_MS, no sudden jump
    applyTarget(BOOT_FADE_MS);
  } else {
    // Start completely dark
    fade_to(0.0f, 0.0f, 0);
  }

  Serial.println("[lamp] init");
  printState();
}

void lamp_update() {
  // Poll for any new touch event
  const TouchEvent ev = touch_get_event();
  if (ev == TouchEvent::NONE) {
    return;
  }

  switch (ev) {
    case TouchEvent::SINGLE: {
      // Toggle ON/OFF
      s_isOn = !s_isOn;
      applyTarget(FADE_MS_USER);
      syncSettings();
      printState();
      break;
    }

    case TouchEvent::DOUBLE: {
      if (!s_isOn) {
        Serial.println("[lamp] ignored DOUBLE (lamp is OFF)");
        break;
      }
      // Cycle brightness levels: 25% -> 50% -> 75% -> 100% -> 25%...
      s_levelIndex = (s_levelIndex + 1) % BRIGHTNESS_LEVEL_COUNT;
      applyTarget(FADE_MS_USER);
      syncSettings();
      printState();
      break;
    }

    case TouchEvent::TRIPLE: {
      if (!s_isOn) {
        Serial.println("[lamp] ignored TRIPLE (lamp is OFF)");
        break;
      }
      // Cycle colour presets: WARM -> NEUTRAL -> COOL -> WARM...
      if (s_preset == LampPreset::WARM) {
        s_preset = LampPreset::NEUTRAL;
      } else if (s_preset == LampPreset::NEUTRAL) {
        s_preset = LampPreset::COOL;
      } else {
        s_preset = LampPreset::WARM;
      }
      applyTarget(FADE_MS_USER);
      syncSettings();
      printState();
      break;
    }

    case TouchEvent::LONG: {
      // Toggle night mode
      s_nightMode = !s_nightMode;
      if (s_nightMode) {
        s_prevPreset = s_preset;
        s_preset = LampPreset::WARM;
      } else {
        s_preset = s_prevPreset;
      }
      // If lamp is ON, transition smoothly over FADE_MS_NIGHT (1500 ms)
      if (s_isOn) {
        applyTarget(FADE_MS_NIGHT);
      }
      syncSettings();
      printState();
      break;
    }

    default:
      break;
  }
}

bool lamp_is_on() {
  return s_isOn;
}

uint8_t lamp_get_level_index() {
  return s_levelIndex;
}

uint8_t lamp_get_brightness_percent() {
  return BRIGHTNESS_LEVELS[s_levelIndex];
}

LampPreset lamp_get_preset() {
  return s_preset;
}

bool lamp_is_night_mode() {
  return s_nightMode;
}
