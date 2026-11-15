#ifndef LAMP_H
#define LAMP_H

#include <stdint.h>
#include "config.h"

/**
 * Initialize the lamp controller module.
 * Initial state at boot: OFF, default level (50%), NEUTRAL preset, night mode OFF.
 */
void lamp_init();

/**
 * Periodic update for the lamp logic, called every loop() iteration.
 * Polls touch events and applies brightness pipeline changes.
 */
void lamp_update();

/**
 * Check if the lamp is currently switched ON.
 */
bool lamp_is_on();

/**
 * Get the current brightness percentage value (e.g. 25, 50, 75, 100).
 */
uint8_t lamp_level_percent();
uint8_t lamp_get_brightness_percent();

/**
 * Get the current 0-based brightness level index (0 to BRIGHTNESS_LEVEL_COUNT - 1).
 */
uint8_t lamp_get_level_index();

/**
 * Get the active colour preset (WARM, NEUTRAL, COOL).
 */
LampPreset lamp_get_preset();

/**
 * Get human-readable name of the current colour preset ("WARM", "NEUTRAL", "COOL").
 */
const char* lamp_preset_name();

/**
 * Check if night mode is currently enabled.
 */
bool lamp_night_mode();
bool lamp_is_night_mode();

#endif  // LAMP_H
