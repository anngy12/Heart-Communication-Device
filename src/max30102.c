#include "max30102.h"

uint8_t max30102_read_reg(uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MAX30102_ADDR, &val, 1, false);
    return val;
}

uint8_t max30102_fifo_available(void) {
    uint8_t write_ptr = max30102_read_reg(0x04);
    uint8_t read_ptr  = max30102_read_reg(0x06);
    return (write_ptr >= read_ptr) ? (write_ptr - read_ptr) : (32 + write_ptr - read_ptr);
}

void max30102_init(void) {
    float bpm_target = 90.0f;  // Default, bis erste Messung kommt
    bool finger_on = false;

    // I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    busy_wait_ms(200);
    uint8_t reset[2] = {0x09, 0x40};
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, reset, 2, false);
    busy_wait_ms(100);

    uint8_t fifo_cfg[2] = {0x08, 0x4F}; // FIFO: Sample Avg=4, Rollover Enable
    uint8_t mode_cfg[2] = {0x09, 0x03}; // Mode: SpO2 (RED+IR)
    uint8_t spO2_cfg[2] = {0x0A, 0x27}; // SPO2_ADC_RGE=4096nA, SR=100Hz, LED_PW=411us(18-bit)
    uint8_t led_red[2]  = {0x0C, 0x10}; // Low current defaults
    uint8_t led_ir[2]   = {0x0D, 0x10};

    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, fifo_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, mode_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, spO2_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, led_red,  2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, led_ir,   2, false);
}

uint32_t max30102_read_ir_sample(void) {
    uint8_t reg = 0x07; // FIFO_DATA
    uint8_t buf[6];
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
    int r = i2c_read_blocking(I2C_PORT, MAX30102_ADDR, buf, 6, false);
    if (r != 6) return 0;
    // Bytes 3..5 sind IR (18bit)
    uint32_t ir = ((uint32_t)(buf[3] & 0x03) << 16) | (buf[4] << 8) | buf[5];
    return ir;
}


