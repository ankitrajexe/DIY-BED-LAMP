#include "settings.h"

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// In C++, the unnamed namespace keeps variables private to this file
// (equivalent to module-level private variables in Python).
namespace {

// In-memory active settings
Settings s_current;

// Copy of what is currently written in flash (to prevent redundant writes)
Settings s_saved;

// Dirty flag and timestamp of the last user adjustment
bool s_isDirty = false;
uint32_t s_dirtyTimestampMs = 0;

// Preferences NVS namespace (max 15 chars)
const char* NVS_NAMESPACE = "lamp_cfg";

/**
 * Compare two Settings structs to check if any field actually changed.
 */
bool settingsDiffer(const Settings& a, const Settings& b) {
  return (a.lamp_on != b.lamp_on) ||
         (a.level_index != b.level_index) ||
         (a.preset != b.preset) ||
         (a.night_mode != b.night_mode) ||
         (a.last_seen_date != b.last_seen_date);
}

/**
 * Write in-memory settings to ESP32 flash memory using the Preferences library.
 */
void writeToFlash() {
  Preferences prefs;
  if (!prefs.begin(NVS_NAMESPACE, false)) {  // false = read/write mode
    Serial.println("[settings] error: could not open flash for writing");
    return;
  }

  prefs.putUShort("ver", SETTINGS_VERSION);
  prefs.putBool("on", s_current.lamp_on);
  prefs.putUChar("lvl", s_current.level_index);
  prefs.putUChar("pre", static_cast<uint8_t>(s_current.preset));
  prefs.putBool("night", s_current.night_mode);
  prefs.putUInt("date", s_current.last_seen_date);
  prefs.end();

  s_saved = s_current;
  s_isDirty = false;
  Serial.println("[settings] saved");
}

}  // namespace

void settings_load_defaults() {
  s_current.lamp_on = false;
  s_current.level_index = BRIGHTNESS_DEFAULT_LEVEL_INDEX;
  s_current.preset = LampPreset::NEUTRAL;
  s_current.night_mode = false;
  s_current.last_seen_date = 0;
  s_saved = s_current;
  s_isDirty = false;
}

void settings_init() {
  Preferences prefs;
  // Attempt to open in read-only mode first
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    // Namespace does not exist yet (first boot on fresh chip)
    settings_load_defaults();
    Serial.println("[settings] defaults (first boot or version mismatch)");
    return;
  }

  // Check stored schema version
  const uint16_t storedVersion = prefs.getUShort("ver", 0);
  if (storedVersion != SETTINGS_VERSION) {
    prefs.end();
    settings_load_defaults();
    Serial.println("[settings] defaults (first boot or version mismatch)");
    return;
  }

  // Version matches: load saved values
  s_current.lamp_on = prefs.getBool("on", false);
  s_current.level_index = prefs.getUChar("lvl", BRIGHTNESS_DEFAULT_LEVEL_INDEX);
  s_current.preset = static_cast<LampPreset>(prefs.getUChar("pre", static_cast<uint8_t>(LampPreset::NEUTRAL)));
  s_current.night_mode = prefs.getBool("night", false);
  s_current.last_seen_date = prefs.getUInt("date", 0);
  prefs.end();

  // Defensive sanity checks on loaded data
  if (s_current.level_index >= BRIGHTNESS_LEVEL_COUNT) {
    s_current.level_index = BRIGHTNESS_DEFAULT_LEVEL_INDEX;
  }
  if (s_current.preset != LampPreset::WARM &&
      s_current.preset != LampPreset::NEUTRAL &&
      s_current.preset != LampPreset::COOL) {
    s_current.preset = LampPreset::NEUTRAL;
  }

  s_saved = s_current;
  s_isDirty = false;
  Serial.println("[settings] loaded");
}

void settings_update() {
  // If no unsaved modifications, nothing to do
  if (!s_isDirty) {
    return;
  }

  // Wait until SETTINGS_SAVE_DELAY_MS (5000 ms) have passed since the LAST change
  const uint32_t now = millis();
  if ((now - s_dirtyTimestampMs) >= SETTINGS_SAVE_DELAY_MS) {
    // Check if the values actually differ from what is already in flash
    if (settingsDiffer(s_current, s_saved)) {
      writeToFlash();
    } else {
      // Reverted to same values as saved; no flash write needed
      s_isDirty = false;
    }
  }
}

void settings_mark_dirty() {
  s_isDirty = true;
  s_dirtyTimestampMs = millis();
}

const Settings& settings_get() {
  return s_current;
}

Settings& settings_get_mutable() {
  return s_current;
}

// ---------------------------------------------------------------------------
// Step 6 Serial test helper (remove in Step 19)
// ---------------------------------------------------------------------------
void settings_factory_reset() {
  Preferences prefs;
  if (prefs.begin(NVS_NAMESPACE, false)) {
    prefs.clear();
    prefs.end();
  }
  settings_load_defaults();
  Serial.println("[settings] factory reset: cleared flash and loaded defaults");
}
