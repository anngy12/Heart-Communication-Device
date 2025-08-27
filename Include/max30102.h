#ifndef MAX30102_H
#define MAX30102_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define MAX30102_ADDR 0x57
#define FINGER_ON_THRESHOLD 50000

void max30102_init();
uint8_t read_register(uint8_t reg);
uint8_t fifo_available_samples();
uint32_t max30102_read_ir();
bool max30102_finger_present(uint32_t ir_avg);

#endif
