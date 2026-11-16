#ifndef FADE_H
#define FADE_H

#include <stdint.h>

/**
 * Initialize the fade engine.
 * Sets both channels to 0.0 brightness.
 */
void fade_init();

/**
 * Start a smooth non-blocking transition to target brightness levels.
 *
 * If a fade is already in progress, the new fade starts smoothly from the
 * current intermediate brightness instead of jumping back to the old start.
 *
 * @param targetWarm Target warm channel brightness (0.0 to 1.0).
 * @param targetCool Target cool channel brightness (0.0 to 1.0).
 * @param durationMs Duration of transition in milliseconds (0 = immediate).
 */
void fade_to(float targetWarm, float targetCool, uint32_t durationMs);

/**
 * Periodic update for the fade engine, called every loop() iteration.
 * Updates LED values at approximately 50 Hz (every FADE_INTERVAL_MS).
 */
void fade_update();

/**
 * Check if a fade is currently in progress.
 *
 * @return true if actively fading, false if stationary.
 */
bool fade_is_active();

/**
 * Get current interpolated warm channel brightness.
 *
 * @return Warm level (0.0 to 1.0).
 */
float fade_get_warm();

/**
 * Get current interpolated cool channel brightness.
 *
 * @return Cool level (0.0 to 1.0).
 */
float fade_get_cool();

/**
 * Handle fade test commands ("f <warm> <cool> <ms>", "factory").
 * Returns true if recognized, false otherwise.
 * Remove in Step 19.
 */
bool fade_handle_command(const char* cmd);

#endif  // FADE_H
