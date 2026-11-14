#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Serial debug baud rate and startup USB CDC wait timeout (0 to disable wait)
constexpr uint32_t SERIAL_BAUD    = 115200;
constexpr uint32_t SERIAL_WAIT_MS = 2000;

// LED PWM driver (ESP32 Arduino core 3.x LEDC)
constexpr uint32_t LED_PWM_FREQ_HZ       = 20000;
constexpr uint8_t  LED_PWM_RES_BITS      = 10;
constexpr float    LED_GAMMA             = 2.2f;

// LED Step 2 serial test helpers (remove in Step 19)
constexpr uint32_t LED_SWEEP_DURATION_MS = 10000;
constexpr uint32_t LED_SWEEP_STEP_MS     = 50;
constexpr uint8_t  LED_CMD_BUF_SIZE      = 32;

// Fade engine settings (Step 3)
constexpr uint32_t FADE_INTERVAL_MS = 20;      // 50 Hz update interval (1000 ms / 50 = 20 ms)
constexpr uint32_t FADE_MS_USER     = 300;     // Fast fade for user taps / level changes
constexpr uint32_t FADE_MS_AUTO     = 2000;    // Gentle 2 s fade for ambient light adjustments
constexpr uint32_t FADE_MS_NIGHT    = 1500;    // 1.5 s transition when entering/exiting night mode
constexpr uint32_t FADE_MS_SUNRISE  = 300000;  // 5 min gradual ramp for sunrise alarm

// Touch input settings (Step 4, TTP223 active HIGH on TOUCH_PIN)
constexpr uint32_t TOUCH_DEBOUNCE_MS = 30;   // Noise debounce window (30 ms)
constexpr uint32_t TAP_WINDOW_MS    = 300;  // Max gap between multi-taps (300 ms)
constexpr uint32_t LONG_PRESS_MS    = 800;  // Hold duration to trigger LONG event (800 ms)

// Lamp brightness & preset settings (Step 5)
constexpr uint8_t BRIGHTNESS_LEVELS[]            = {25, 50, 75, 100};
constexpr uint8_t BRIGHTNESS_LEVEL_COUNT         = sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]);
constexpr uint8_t BRIGHTNESS_DEFAULT_LEVEL_INDEX = 1;   // Default 50%
constexpr uint8_t MIN_BRIGHTNESS_PERCENT         = 5;   // Never below 5% when ON
constexpr uint8_t NIGHT_BRIGHTNESS_PERCENT       = 35;  // Brightness cap in night mode

// Colour presets as warm/cool channel mixes (0.0 to 1.0)
struct ColorPresetMix {
  float warm;
  float cool;
};

enum class LampPreset : uint8_t {
  WARM = 0,
  NEUTRAL = 1,
  COOL = 2
};

constexpr ColorPresetMix PRESET_WARM    = {1.0f, 0.0f};
constexpr ColorPresetMix PRESET_NEUTRAL = {0.6f, 0.6f};
constexpr ColorPresetMix PRESET_COOL    = {0.0f, 1.0f};

// GPIO pin assignments (ESP32-C3 SuperMini)
// Never use GPIO8 and GPIO9 (strapping pins).
constexpr uint8_t LED_WARM_PIN = 0;  // GPIO0: warm channel MOSFET gate (PWM)
constexpr uint8_t LED_COOL_PIN = 1;  // GPIO1: cool channel MOSFET gate (PWM)
constexpr uint8_t TOUCH_PIN    = 3;  // GPIO3: TTP223 touch output (active HIGH)
constexpr uint8_t I2C_SDA_PIN  = 4;  // GPIO4: I2C SDA
constexpr uint8_t I2C_SCL_PIN  = 5;  // GPIO5: I2C SCL
constexpr uint8_t BUTTON_PIN   = 6;  // GPIO6: page button to GND (INPUT_PULLUP, active LOW)

// I2C device addresses (shared 3.3V bus)
constexpr uint8_t BH1750_I2C_ADDR  = 0x23;  // Ambient light sensor
constexpr uint8_t AHT20_I2C_ADDR   = 0x38;  // Temperature and humidity sensor
constexpr uint8_t SSD1306_I2C_ADDR = 0x3C;  // 128x64 OLED display
constexpr uint8_t DS3231_I2C_ADDR  = 0x68;  // RTC (module EEPROM at 0x57 is ignored)

#endif  // CONFIG_H
