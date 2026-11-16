#ifndef SERIAL_CMD_H
#define SERIAL_CMD_H

/**
 * Isolated Serial command dispatcher for development & testing.
 * Reads full lines from Serial Monitor, trims whitespace and carriage returns,
 * and hands the command to module handlers (led, fade, rtc, i2c scan).
 * Only if none recognize it, prints a single "unknown cmd" error.
 * Remove this file entirely in Step 19.
 */
void serial_cmd_update();

#endif  // SERIAL_CMD_H
