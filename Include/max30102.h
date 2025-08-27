#ifndef MAX30102_H
#define MAX30102_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdio.h>

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define MAX30102_ADDR 0x57

void max30102_init(void);
uint8_t max30102_read_reg(uint8_t reg);
uint8_t max30102_fifo_available(void);
/** Liest 1 Samplesatz (nur IR) aus FIFO (6 Byte). Gibt 18-bit IR zurück oder 0 bei Fehler. */
uint32_t max30102_read_ir_sample(void);

#endif
