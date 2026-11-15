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
