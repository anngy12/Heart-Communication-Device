#ifndef MAX30102_H
#define MAX30102_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define MAX30102_ADDR 0x57

// ---- BPM-Detektions-Parameter ----
#define MOVING_AVG_SIZE      8
#define BPM_AVG_SIZE         4
#define MIN_VALID_BPM        60
#define MAX_VALID_BPM        180
#define MIN_PEAK_INTERVAL_MS 500
#define FINGER_ON_THRESHOLD  50000  // IR-Durchschnittsschwelle

void max30102_init(void);
uint8_t max30102_read_reg(uint8_t reg);
uint8_t max30102_fifo_available(void);
uint32_t max30102_read_ir_sample(void);

#endif
