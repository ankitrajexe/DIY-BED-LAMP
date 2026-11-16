#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include "config.h"

/**
 * Settings struct holding all user-changeable parameters.
 * In Python, this is like a dataclass:
 *   @dataclass
 *   class Settings:
 *       lamp_on: bool
 *       level_index: int
 *       preset: LampPreset
 *       night_mode: bool
 *
 * Later steps will add more fields (e.g. Pomodoro times, sunrise alarm).
 */
struct Settings {
  bool       lamp_on;         // Whether lamp was on when last saved
  uint8_t    level_index;     // Brightness level index (0..3)
  LampPreset preset;          // Active colour preset (WARM, NEUTRAL, COOL)
  bool       night_mode;      // Whether night mode is active
  uint32_t   last_seen_date;  // yyyymmdd (e.g. 20261116) for date-changed detection
};

/**
 * Initialize the settings module at boot.
 * Reads flash (Preferences). If SETTINGS_VERSION matches, loads saved values.
 * If first boot or version mismatch, loads defaults from config.h.
 */
void settings_init();

/**
 * Periodic update called on every loop() iteration.
 * Non-blocking: writes to flash only when dirty AND 5 s have passed since last change.
 */
void settings_update();

/**
 * Mark settings as modified.
 * Starts / resets the 5-second countdown timer before writing to flash.
 */
void settings_mark_dirty();

/**
 * Read-only access to current settings in RAM.
 */
const Settings& settings_get();

/**
 * Mutable access to current settings in RAM.
 * Call settings_mark_dirty() after modifying values.
 */
Settings& settings_get_mutable();

/**
 * Reset settings in RAM to config.h defaults.
 */
void settings_load_defaults();

/**
 * Step 6 Serial test helper: clears saved flash and reloads defaults.
 * Can be removed in Step 19.
 */
void settings_factory_reset();

#endif  // SETTINGS_H
