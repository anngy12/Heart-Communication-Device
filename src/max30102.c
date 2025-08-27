#include "max30102.h"

uint8_t read_register(uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MAX30102_ADDR, &val, 1, false);
    return val;
}

uint8_t fifo_available_samples() {
    uint8_t write_ptr = read_register(0x04);
    uint8_t read_ptr = read_register(0x06);
    return (write_ptr >= read_ptr) ? (write_ptr - read_ptr) : (32 + write_ptr - read_ptr);
}

void max30102_init() {
    uint8_t reset[2] = { 0x09, 0x40 };
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, reset, 2, false);
    busy_wait_ms(100);

    uint8_t fifo_cfg[2] = { 0x08, 0x4F };
    uint8_t mode_cfg[2] = { 0x09, 0x03 };
    uint8_t spO2_cfg[2] = { 0x0A, 0x27 };
    uint8_t led_red[2] = { 0x0C, 0x10 };
    uint8_t led_ir[2]  = { 0x0D, 0x10 };

    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, fifo_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, mode_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, spO2_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, led_red, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, led_ir, 2, false);
}

uint32_t max30102_read_ir() {
    uint8_t reg = 0x07;
    uint8_t buffer[6];
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
    int read_bytes = i2c_read_blocking(I2C_PORT, MAX30102_ADDR, buffer, 6, false);
    if (read_bytes == 6) {
        uint32_t ir = ((uint32_t)(buffer[3] & 0x03) << 16) | (buffer[4] << 8) | buffer[5];
        return ir;
    }
    return 0;
}

bool max30102_finger_present(uint32_t ir_avg) {
    return ir_avg > FINGER_ON_THRESHOLD;
}
