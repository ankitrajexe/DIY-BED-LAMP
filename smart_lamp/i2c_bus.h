#ifndef I2C_BUS_H
#define I2C_BUS_H

/**
 * Central initialization for the shared I2C bus.
 * Configures GPIO4 (SDA) and GPIO5 (SCL) and sets the clock to 400 kHz (I2C_CLOCK_HZ).
 * This is the ONLY place in the codebase that configures the I2C bus parameters.
 */
void i2c_bus_init();

/**
 * Scan the I2C bus and print every device address found.
 * Helpful for verifying 0x3C (OLED), 0x57 (EEPROM), and 0x68 (DS3231).
 */
void i2c_scan();

#endif  // I2C_BUS_H
