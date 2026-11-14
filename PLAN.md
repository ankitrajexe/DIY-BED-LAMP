# PLAN.md: USB-C Smart Reading Lamp (v2)

## How to work (read first)
- Do ONLY the step I name. When it is done, stop and tell me: files changed, how to compile, how to test, what I should see. Wait for my confirmation before the next step.
- Before each step, read the current code and tell me in 3 lines or less if the step clashes with existing code, pins, timing or structure. If it does, suggest a fix. Do not silently deviate from this plan.
- Do not build features from later steps early. Do not refactor earlier steps unless I ask.
- Beginner friendly comments everywhere. I only know Python at class level, so explain anything non-obvious in C++ in a short comment.
- If something is unclear or conflicts with the hardware, ask. Do not guess.

## Status
- Step 1 DONE (skeleton, flashed, Serial logs verified).
- Step 2 code DONE (LED driver), waiting for my hardware test confirmation.

## Hardware
- Board: ESP32-C3 SuperMini. Arduino framework, Arduino IDE compatible, multiple .h/.cpp files.
- Arduino IDE settings: Board "ESP32C3 Dev Module", "USB CDC On Boot" Enabled. ESP32 Arduino core 3.x (LEDC API: ledcAttach, ledcWrite).
- Pins: GPIO0 warm PWM, GPIO1 cool PWM, GPIO3 TTP223 touch (active HIGH), GPIO4 SDA, GPIO5 SCL, GPIO6 page button to GND (INPUT_PULLUP, active LOW). Never use GPIO8 and GPIO9.
- I2C devices on one bus (3.3V): BH1750 0x23, AHT20 0x38, SSD1306 OLED 0x3C, DS3231 0x68 (its EEPROM at 0x57 is ignored).
- I2C clock is 400 kHz, set in ONE place only (I2C_CLOCK_HZ in config.h). A full OLED frame is about 23 ms at 400 kHz, so the face runs at about 20 FPS max, not 60.
- LED: 5V CCT strip, two low-side IRLZ44N MOSFETs (gate 100 ohm series + 10k pulldown). Power: USB-C 5V, no battery. DS3231 has a CR2032.
- No new hardware beyond this list. There is no current sensor, so "power" on screen is only an ESTIMATE from PWM duty.
- The BH1750 can see the lamp's own light (handled in Step 11).

## Rules
- Modular. One module per feature, clear function names, everything hooked into one central app_init() and app_update() flow.
- No delay() anywhere. Only millis() based non-blocking logic. Touch, fades, animation and logging must all work together.
- All pins, I2C addresses, timings, thresholds and defaults live in config.h only. No magic numbers elsewhere.
- Serial debug log in every module.
- Every optional device (BH1750, AHT20, OLED, DS3231) fails gracefully: log once and skip. The lamp must always work with just LEDs and touch.
- Prefer these libraries: U8g2 (OLED, full buffer mode), RTClib (DS3231), Adafruit AHTX0 (AHT20), BH1750 by Christopher Laws, Preferences (flash). List all libraries in the README.
- Keep SERIAL_WAIT_MS in config.h. Remind me to set it to 0 for the final build.

## Architecture rules (these apply from Step 3 onwards)
1. One brightness pipeline. lamp.cpp has a single lamp_compute_target() that builds the LED target from layers in this order: (a) user level and colour preset, (b) auto brightness from BH1750, (c) night factor, (d) Pomodoro break dim, (e) sunrise alarm override. Features only change the inputs of their own layer. Nothing else writes to the LEDs directly. Every change goes through the fade engine.
2. One settings struct. settings.h/.cpp holds a Settings struct. Every user-changeable value has a default in config.h, settings_load() at boot, settings_mark_dirty() on change, and a flash write 5 s after the last change. Add a settings version number; if it does not match, load defaults. Later steps only add fields.
3. Time comes from the rtc module: rtc_now(), plus a "date changed" flag that compares the current date with the last seen date. This also fires on boot if the date changed while the lamp was off. Min/max log and study hours use this, not a midnight timer.
4. Flash wear: logs live in RAM and are written to flash at most every LOG_FLASH_WRITE_MIN minutes (default 10) and on date change. Never write flash every loop or every second.
5. Display is split: display.h/.cpp is only the driver (init, frame, contrast, sleep, pixel shift, frame limit). Each page lives in its own file (page_clock.cpp, page_env.cpp, ...) with one draw function. Normal pages refresh at 5 Hz, face at FACE_FPS.
6. The menu is table driven: each setting is one table row (name, min, max, step, pointer into Settings). Later steps add rows, never new menu code.

## Inputs (final gesture map, defaults)
Touch pad (TTP223): single tap = lamp on/off. Double tap = cycle brightness levels (default 4 levels: 25, 50, 75, 100 percent). Triple tap = cycle colour preset warm, neutral, cool. Long press (LONG_PRESS_MS = 800, fires once) = toggle night mode. A single tap is reported only after the tap window closes (300 ms).
Push button (GPIO6): short press = next OLED page. Long press (1 s) = open settings menu. Double press on the Timer page = start or stop Pomodoro.
Inside the menu: button short press = next item, touch tap = change value, button long press = save and exit. Auto exit after 30 s with no input.
Any touch or button press wakes the OLED. The first press only wakes it and does nothing else.

## OLED pages (final)
1. Clock: time large, date and weekday.
2. Environment: temperature and humidity now, today's min and max.
3. Lamp: mode, brightness %, auto or manual, lux, ESTIMATED power (from duty and a calibration constant in config.h).
4. Timer: Pomodoro countdown and state.
5. Study: bar graph of lamp-on hours for the last 7 days, today's total.
6. Face.
A page whose device is missing shows "not found". After the display wakes from sleep, start at page 1.

## Steps

### Step 1: Skeleton (DONE)

### Step 2: LED driver (code DONE)
LEDC PWM 20 kHz, 10-bit, led_set(warm, cool) with gamma 2.2, Serial test commands. Keep the test commands in one block so they can be removed in Step 19.

### Step 3: Fade engine
Non-blocking fades: fade_to(targetWarm, targetCool, durationMs), fade_update() every loop at about 50 Hz. A new target mid-fade starts from the current value, never from the old start. Fade durations as config values (FADE_MS_USER, FADE_MS_AUTO, FADE_MS_NIGHT, FADE_MS_SUNRISE). No visible jumps.

### Step 4: Touch input
TTP223 state machine with debounce. Events: single tap, double tap, triple tap, long press (fires once). No LED logic in this module. Print events on Serial.

### Step 5: Lamp logic
Implement the touch gesture map above. Brightness levels and colour presets (warm, neutral, cool as warm/cool channel mixes in config.h). Manual night mode toggle (night factor from Step 10 is applied here as a placeholder cap). MIN_BRIGHTNESS: never fully off while the lamp is ON. Build lamp_compute_target() with the layers from the architecture rules, empty layers pass through.

### Step 6: Settings and persistence
settings.h/.cpp as described. Save: on/off state, level index, colour preset, night mode. Restore at boot. 5 s save delay after last change.

### Step 7: OLED driver
SSD1306 128x64 at 0x3C, 400 kHz, U8g2 full buffer. Sleep after OLED_SLEEP_MS with no input, wake on any touch or button event. Burn-in protection: shift static content by 1 to 2 pixels every PIXEL_SHIFT_INTERVAL_MS (default 60 s). Contrast value in config.h. For now show one simple lamp status screen. If OLED not found, skip silently.

### Step 8: RTC (DS3231)
rtc_init(), rtc_now(), set from compile time on power loss, Serial command "T YYYY-MM-DD HH:MM:SS" to set the time, date-changed flag (architecture rule 3). If RTC not found, skip silently.

### Step 9: Push button and page framework
button.h/.cpp: debounce, short press, long press, double press events. Page manager with page_*.cpp files. Pages added now: Clock and Lamp. The other pages show "coming soon" until their step. Menu long press is only logged for now.

### Step 10: Auto night dimming
NIGHT_START_HOUR (default 23) and NIGHT_END_HOUR (default 5) in config.h and Settings. Inside that window: cap brightness at NIGHT_BRIGHTNESS_PERCENT (default 35), shift colour to warm, lower OLED contrast, use a slow NIGHT_FADE_MS fade. Manual night mode from the long press uses the same effect. The user can still change levels, but never above the night cap. Without RTC, only manual night mode works.

### Step 11: Auto brightness (BH1750)
light_read_lux() every 500 ms, exponential moving average, hysteresis, 2 s fade. Auto mode is the layer (b). Manual level changes act as an offset on top. Subtract an estimated lamp contribution based on current PWM, calibration constant in config.h. If BH1750 not found, fall back to manual.

### Step 12: Temperature and humidity (AHT20)
env_read() every 5 s, non-blocking. ENV_TEMP_OFFSET_C and ENV_HUM_OFFSET in config.h (the lamp heats the sensor). Environment page (now values only for now). If not found, show "not found".

### Step 13: Pomodoro
Work and break lengths in Settings (default 25 and 5). Countdown on the Timer page. During breaks the lamp dims slightly (POMODORO_BREAK_DIM_PERCENT) through layer (d). Soft brightness pulses at each transition, no sound. Start or stop with button double press on the Timer page. Runs even when the OLED is asleep and does not wake it.

### Step 14: Study hours tracker
Accumulate lamp-on time per day. Keep the last 7 days as a ring buffer keyed by date. RAM accumulation, flash write per architecture rule 4. Study page: 7-day bar graph and today's total.

### Step 15: Min/max log
Today's min and max temperature and humidity, kept in RAM and written to flash per rule 4. Reset on the date-changed flag (also on boot if the date changed). Shown on the Environment page.

### Step 16: Settings menu
Long press on the push button opens it. Table driven (rule 6). Items: brightness levels (choose 3, 4 or 5 evenly spaced), Pomodoro work and break minutes, alarm time and on/off (row added in Step 17), night start hour, OLED sleep timeout. Navigation as in the Inputs section. All values saved through Settings.

### Step 17: Sunrise alarm
Use a software time check on rtc_now() once per second, not the DS3231 alarm registers (the lamp is always powered, so the hardware alarm adds nothing). Alarm time and enabled flag in Settings. Fade from 0 to ALARM_TARGET_PERCENT over ALARM_RAMP_MIN minutes (default 5) so it reaches the target AT the alarm time, through layer (e). If the lamp boots inside the ramp window, start at the matching point. Any tap cancels it. No buzzer. Add the menu rows.

### Step 18: Face mode
New page 6 and also the idle screen: after IDLE_FACE_AFTER_MS with no input show the face, then OLED sleep takes over. In night mode skip the idle face and sleep sooner (shorter timeout in config.h), because a glowing animated screen at night disturbs the room.
Draw with rounded boxes, circles and short line segments only, no bitmaps. U8g2 has no arbitrary arcs, so draw curves (smiles, wavy mouth) as short line segments computed with sin and cos. Frame limit FACE_FPS (default 20). Idle behaviour: random blinks, eyes drift slowly.
Moods and priority (highest first): Wake-up (sunrise alarm active), Excited (on touch tap, heart eyes for 1 s), Break (Pomodoro break), Focus (Pomodoro work), Hot (temp above FACE_HOT_C), Cold (below FACE_COLD_C), Humid (above FACE_HUMID_PCT), Sleepy (night hours, half closed eyes, small z), Bored (no touch for FACE_BORED_MS, looks side to side, one yawn), Happy (default). Use 1 degree or 3 percent hysteresis on the thresholds. Thresholds in config.h.

### Step 19: Cleanup and README (v1 baseline)
Review for blocking code, magic numbers, missing error handling, unused code. Remove or flag the Step 2 test commands. Set SERIAL_WAIT_MS to 0. Write README.md: wiring table, I2C addresses, every gesture and button action, menu items, libraries, test checklist. Add notes: some power banks switch off at very low current (fix is hardware, small dummy load or a power bank with low-current mode, not code), and overnight features like the sunrise alarm need a power source that stays on.

### Step 20 (OPTIONAL, only after Step 19 works on real hardware): WiFi extras
Behind a compile flag ENABLE_WIFI (default 0) and a Settings flag that is OFF by default. WiFi heats the ESP32 and raises the temperature reading.
- NTP sync: connect, set the RTC (IST, UTC+5:30 in config.h), then turn WiFi fully off again.
- Local web page to control the lamp: protected by a PIN, never open. Support a SoftAP mode (the lamp makes its own network) because hostel WiFi may have a login page or block device to device traffic.
- OTA updates: password protected, first flash is always by USB.
- WiFi runs only when the user turns it on from the menu.
