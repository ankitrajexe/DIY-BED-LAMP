#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/**
 * Initialize the SSD1306 128x64 OLED display driver (U8g2 full buffer mode).
 * Probes the I2C bus at SSD1306_I2C_ADDR first.
 * If not found, logs once and becomes a complete no-op (lamp continues working).
 */
void display_init();

/**
 * Periodic non-blocking update, called on every loop() iteration.
 * Manages 5 Hz refresh rate (DISPLAY_REFRESH_MS), sleep timeout (OLED_SLEEP_MS),
 * burn-in pixel shift (PIXEL_SHIFT_INTERVAL_MS), and screen rendering.
 */
void display_update();

/**
 * Check if the display hardware was detected on the I2C bus.
 *
 * @return true if display answered on I2C at boot, false otherwise.
 */
bool display_is_available();

/**
 * Check if the display panel is currently sleeping (power save on).
 *
 * @return true if panel is asleep, false if awake.
 */
bool display_is_asleep();

#endif  // DISPLAY_H
