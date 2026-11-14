#ifndef LED_H
#define LED_H

/** Initialize LEDC PWM on warm and cool pins at zero duty. */
void led_init();

/** Set warm and cool brightness levels (0.0 to 1.0, clamped and gamma-corrected). */
void led_set(float warm, float cool);

/** Periodic non-blocking update for the LED driver module. */
void led_update();

#endif  // LED_H
