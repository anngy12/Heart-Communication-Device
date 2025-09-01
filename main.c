#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// === Konfiguration ===
#define MAX30102_ADDR 0x57
#define I2C_PORT i2c0
#define PIN_SDA  4
#define PIN_SCL  5

#define REG_MODE_CONFIG 0x09
#define REG_LED1_PA     0x0C
#define REG_LED2_PA     0x0D

// Hilfsfunktion: Schreibe ein Register
int max30102_write(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    int ret = i2c_write_blocking(I2C_PORT, MAX30102_ADDR, buf, 2, false);
    return ret; // >=0 = OK, -1 = Fehler
}

// Hilfsfunktion: Lese ein Register
int max30102_read(uint8_t reg, uint8_t *value) {
    int ret = i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
    if (ret < 0) return ret;
    ret = i2c_read_blocking(I2C_PORT, MAX30102_ADDR, value, 1, false);
    return ret;
}

// Initialisierung: Sensor zurücksetzen und LEDs einschalten
void max30102_init() {
    printf("Sensor Reset...\n");
    max30102_write(REG_MODE_CONFIG, 0x40); // Reset
    sleep_ms(500); // etwas länger warten

    printf("SpO2 Mode + LEDs konfigurieren...\n");
    max30102_write(REG_MODE_CONFIG, 0x03); // SpO2 Mode (RED+IR)
    max30102_write(REG_LED1_PA, 0x24);     // RED LED ~7mA
    max30102_write(REG_LED2_PA, 0x24);     // IR LED ~7mA
}

int main() {
    stdio_init_all();

    printf("MAX30102 Testprogramm\n");

    // I2C initialisieren
    i2c_init(I2C_PORT, 400 * 1000); // 400kHz
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    // Test: Sensor ansprechen
    uint8_t val;
    if (max30102_read(REG_MODE_CONFIG, &val) < 0) {
        printf("Fehler: MAX30102 nicht gefunden! Prüfe SDA/SCL und VCC/GND\n");
        while(1) tight_loop_contents();
    }

    printf("Sensor gefunden! REG_MODE_CONFIG = 0x%02X\n", val);

    // Sensor initialisieren
    max30102_init();

    // Testloop: Register regelmäßig auslesen
    while (1) {
        if (max30102_read(REG_MODE_CONFIG, &val) >= 0) {
            printf("REG_MODE_CONFIG = 0x%02X\n", val);
        } else {
            printf("Fehler beim Lesen!\n");
        }
        sleep_ms(1000);
    }

    return 0;
}
