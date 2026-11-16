#include "i2c_bus.h"

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

void i2c_bus_init() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(I2C_CLOCK_HZ);
  Serial.printf("[i2c] bus init (SDA=%d SCL=%d clock=%luHz)\n",
                I2C_SDA_PIN,
                I2C_SCL_PIN,
                static_cast<unsigned long>(I2C_CLOCK_HZ));
}

void i2c_scan() {
  Serial.println("[i2c] scanning bus (0x01 to 0x7E)...");
  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    const uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("[i2c] device found at 0x%02X\n", addr);
      count++;
    }
  }
  if (count == 0) {
    Serial.println("[i2c] no devices found");
  } else {
    Serial.printf("[i2c] scan complete: %u device(s) found\n", count);
  }
}
