#ifndef MAX_INIT_H
#define MAX_INIT_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#define MAX30102_ADDR 0x57
#define I2C_PORT i2c0
#define PIN_SDA  4
#define PIN_SCL  5

// Register des Sensors 
#define REG_MODE_CONFIG   0x09
#define REG_SPO2_CONFIG   0x0A
#define REG_LED1_PA       0x0C
#define REG_LED2_PA       0x0D
#define REG_FIFO_WR_PTR   0x04
#define REG_OVF_COUNTER   0x05
#define REG_FIFO_RD_PTR   0x06
#define REG_FIFO_DATA     0x07

void max30102_write(uint8_t reg, uint8_t value);

uint8_t max30102_read_reg(uint8_t reg);

void max30102_init();

bool max30102_read_sample(uint32_t *red, uint32_t *ir);

#endif // MAX_INIT_H