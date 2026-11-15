#include "display.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "activity.h"
#include "config.h"
#include "lamp.h"

// In C++, the unnamed namespace keeps variables private to this file
// (equivalent to module-level private variables in Python).
namespace {

// U8g2 instance for SSD1306 128x64 I2C in Full Buffer mode (_F_)
// U8X8_PIN_NONE indicates no dedicated hardware reset pin is wired to the display.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Flag indicating whether the SSD1306 answered on the I2C bus
bool s_displayFound = false;

// Sleep state flag (true = panel powered down via setPowerSave)
bool s_isAsleep = false;

// Timestamp of the last frame rendering (for 5 Hz refresh rate throttling)
uint32_t s_lastRenderMs = 0;

/**
 * Draw the Step 7 status screen:
 * Displays lamp state (ON/OFF), brightness percent, preset, night indicator, and MANUAL mode.
 *
 * @param ox Pixel shift X offset for burn-in protection (0, 1, or 2)
 * @param oy Pixel shift Y offset for burn-in protection (0, 1, or 2)
 */
void drawLampStatus(int8_t ox, int8_t oy) {
  // --- Header Bar ---
  u8g2.setFont(u8g2_font_helvB08_tr);
  u8g2.drawStr(ox + 2, oy + 10, "SMART LAMP");

  // Mode badge on top right: "MANUAL" (or "NIGHT" if night mode active)
  if (lamp_night_mode()) {
    u8g2.drawStr(ox + 78, oy + 10, "NIGHT");
  } else {
    u8g2.drawStr(ox + 72, oy + 10, "MANUAL");
  }

  // Horizontal divider line
  u8g2.drawHLine(ox + 0, oy + 13, 126);

  // --- Large State Text: ON or OFF ---
  u8g2.setFont(u8g2_font_helvB14_tr);
  if (lamp_is_on()) {
    u8g2.drawStr(ox + 4, oy + 33, "ON");
  } else {
    u8g2.drawStr(ox + 4, oy + 33, "OFF");
  }

  // --- Brightness Percentage ---
  u8g2.setFont(u8g2_font_helvB10_tr);
  char brightBuf[12];
  snprintf(brightBuf, sizeof(brightBuf), "%u%%", lamp_level_percent());
  u8g2.drawStr(ox + 58, oy + 32, brightBuf);

  // --- Preset Name ---
  u8g2.setFont(u8g2_font_6x10_tf);
  char presetBuf[24];
  snprintf(presetBuf, sizeof(presetBuf), "Preset: %s", lamp_preset_name());
  u8g2.drawStr(ox + 4, oy + 48, presetBuf);

  // --- Bottom Status Bar ---
  if (lamp_night_mode()) {
    u8g2.drawStr(ox + 4, oy + 60, "Night cap: 35% warm");
  } else if (!lamp_is_on()) {
    u8g2.drawStr(ox + 4, oy + 60, "Tap to turn ON");
  } else {
    u8g2.drawStr(ox + 4, oy + 60, "Double:Lvl  Triple:Col");
  }
}

}  // namespace

void display_init() {
  // 1. Probe the I2C bus first to verify SSD1306 presence
  Wire.beginTransmission(SSD1306_I2C_ADDR);
  const uint8_t err = Wire.endTransmission();
  if (err != 0) {
    s_displayFound = false;
    Serial.printf("[display] SSD1306 not found at 0x%02X (error=%d), skipping\n",
                  SSD1306_I2C_ADDR, err);
    return;
  }

  // 2. Device found: initialize U8g2
  s_displayFound = true;
  u8g2.setI2CAddress(SSD1306_I2C_ADDR * 2);  // U8g2 expects 8-bit I2C address (0x3C * 2 = 0x78)
  u8g2.begin();

  // Ensure U8g2 keeps the same 400 kHz bus clock configured in i2c_bus_init()
  u8g2.setBusClock(I2C_CLOCK_HZ);

  // Set default daytime contrast (0..255)
  u8g2.setContrast(OLED_CONTRAST_DAY);

  // Clear display and power on
  u8g2.clearBuffer();
  u8g2.sendBuffer();

  s_isAsleep = false;
  s_lastRenderMs = 0;

  Serial.printf("[display] init (SSD1306 128x64 400kHz addr=0x%02X)\n", SSD1306_I2C_ADDR);
}

void display_update() {
  // If display was not detected at boot, do nothing (fails gracefully)
  if (!s_displayFound) {
    return;
  }

  const uint32_t now = millis();
  const uint32_t inactivityMs = now - activity_last_ms();

  // --- Sleep Management ---
  if (!s_isAsleep && inactivityMs >= OLED_SLEEP_MS) {
    // 10 seconds with no input: turn panel off into power save mode
    s_isAsleep = true;
    u8g2.setPowerSave(1);  // 1 = enable power save (panel turns off)
    Serial.println("[display] sleep (power save on)");
    return;
  }

  if (s_isAsleep && inactivityMs < OLED_SLEEP_MS) {
    // Recent input detected: wake panel up
    s_isAsleep = false;
    u8g2.setPowerSave(0);  // 0 = disable power save (panel turns on)
    Serial.println("[display] wake (power save off)");
    // Render immediately on wake
    s_lastRenderMs = 0;
  }

  // If display is currently sleeping, skip drawing to save CPU cycles
  if (s_isAsleep) {
    return;
  }

  // --- Refresh Rate Throttling (max 5 Hz = 200 ms) ---
  if ((now - s_lastRenderMs) < DISPLAY_REFRESH_MS) {
    return;
  }
  s_lastRenderMs = now;

  // --- Burn-in Protection: Pixel Shift ---
  // Cycles through 0, 1, and 2 pixels in x and y every PIXEL_SHIFT_INTERVAL_MS (60 s)
  const uint32_t shiftStep = (now / PIXEL_SHIFT_INTERVAL_MS) % 3;
  const int8_t shiftX = static_cast<int8_t>(shiftStep);
  const int8_t shiftY = static_cast<int8_t>(shiftStep);

  // --- Render Status Frame ---
  u8g2.clearBuffer();
  drawLampStatus(shiftX, shiftY);
  u8g2.sendBuffer();
}

bool display_is_available() {
  return s_displayFound;
}

bool display_is_asleep() {
  return s_isAsleep;
}
