#include <stdio.h>
#include <stdint.h>

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

// Funktion fürs Schreiben in Register
void max30102_write(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, buf, 2, false);
}

// Funktion fürs Lesen aus Register
uint8_t max30102_read_reg(uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MAX30102_ADDR, &val, 1, false);
    return val;
}

void max30102_init() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    max30102_write(REG_MODE_CONFIG, 0x40); // Reset
    sleep_ms(100);
    max30102_write(REG_MODE_CONFIG, 0x03); // SpO2 Mode (RED+IR)
    max30102_write(REG_SPO2_CONFIG, 0x27); // 100Hz, 18bit, 4096nA
    max30102_write(REG_LED1_PA, 0x24);     // RED LED ~7mA
    max30102_write(REG_LED2_PA, 0x24);     // IR LED ~7mA
    max30102_write(REG_FIFO_WR_PTR, 0x00);
    max30102_write(REG_OVF_COUNTER, 0x00);
    max30102_write(REG_FIFO_RD_PTR, 0x00);
}

bool max30102_read_sample(uint32_t *red, uint32_t *ir) {
    uint8_t reg = REG_FIFO_DATA;
    uint8_t buffer[6];
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
    int read = i2c_read_blocking(I2C_PORT, MAX30102_ADDR, buffer, 6, false);
    if (read != 6) return false;

    *red = ((uint32_t)(buffer[0] & 0x03) << 16) | ((uint32_t)buffer[1] << 8) | buffer[2];
    *ir  = ((uint32_t)(buffer[3] & 0x03) << 16) | ((uint32_t)buffer[4] << 8) | buffer[5];
    return true;
}

