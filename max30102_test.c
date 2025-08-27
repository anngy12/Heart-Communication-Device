#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include <stdio.h>

// ===== MAX30102 =====
#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define MAX30102_ADDR 0x57

// ===== Servo Config =====
#define SERVO_COUNT 4
const uint SERVO_PINS[SERVO_COUNT] = {15, 16, 17, 18};

// ===== BPM Detection =====
#define MOVING_AVG_SIZE 8
#define BPM_AVG_SIZE 4
#define MIN_VALID_BPM 60
#define MAX_VALID_BPM 180
#define MIN_PEAK_INTERVAL_MS 500
#define FINGER_ON_THRESHOLD 50000

// ===== Servo State =====
typedef struct {
    uint pin;
    int angle;
    uint64_t last_toggle;
} ServoState;

ServoState servos[SERVO_COUNT];

// ===== MAX30102 Hilfsfunktionen =====
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
    uint8_t reset[2] = {0x09, 0x40};
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, reset, 2, false);
    busy_wait_ms(100);

    uint8_t fifo_cfg[2] = {0x08, 0x4F};
    uint8_t mode_cfg[2] = {0x09, 0x03};
    uint8_t spO2_cfg[2] = {0x0A, 0x27};
    uint8_t led_red[2] = {0x0C, 0x10};
    uint8_t led_ir[2] = {0x0D, 0x10};

    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, fifo_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, mode_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, spO2_cfg, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, led_red, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, led_ir, 2, false);
}

// ===== Servo Funktionen =====
void set_servo_angle(uint pin, float angle) {
    float min = 0.5f, max = 2.5f; // Pulse width in ms
    float pulse_ms = min + (max - min) * (angle / 180.0f);

    uint slice = pwm_gpio_to_slice_num(pin);
    uint32_t wrap = 20000;  // 50 Hz PWM, festgelegt bei Init

    uint32_t level = (pulse_ms / 20.0f) * wrap;
    pwm_set_gpio_level(pin, level);
}


void init_servos() {
    for (int i = 0; i < SERVO_COUNT; i++) {
        gpio_set_function(SERVO_PINS[i], GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(SERVO_PINS[i]);
        pwm_set_clkdiv(slice, 125.0f);   // 1 MHz Clock -> 20,000 ticks = 20 ms
        pwm_set_wrap(slice, 20000);      // 50 Hz
        pwm_set_enabled(slice, true);

        servos[i].pin = SERVO_PINS[i];
        servos[i].angle = 90; // neutral
        servos[i].last_toggle = time_us_64();
        set_servo_angle(SERVO_PINS[i], 90);
    }
}

void update_servos(float bpm) {
    // Falls noch kein BPM berechnet -> Standard 60 BPM
    if (bpm < MIN_VALID_BPM || bpm > MAX_VALID_BPM) bpm = 60.0f;

    // BPM -> Periodendauer (ms pro Umschaltung)
    float interval_ms = 60000.0f / bpm;

    for (int i = 0; i < SERVO_COUNT; i++) {
        uint64_t now = time_us_64();

        // Jeder Servo leicht verzögert
        uint64_t delay_us = i * 3000; // 50 ms versetzt

        if (now > servos[i].last_toggle + (uint64_t)(interval_ms * 1000) + delay_us) {
            servos[i].last_toggle = now;
            servos[i].angle = (servos[i].angle == 0) ? 180 : 0;
            set_servo_angle(servos[i].pin, servos[i].angle);
        }
    }
}

// ===== Main =====
int main() {
    stdio_init_all();

    // I2C init
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    busy_wait_ms(500);

    // Servos init
    init_servos();

    // MAX30102 check
    uint8_t part_id = read_register(0xFF);
    printf("MAX30102 PART ID: 0x%02X\n", part_id);
    if (part_id != 0x15) {
        printf("Sensor nicht erkannt!\n");
        while (true) tight_loop_contents();
    }
    max30102_init();

    // Buffer für BPM
    uint32_t ir_buffer[MOVING_AVG_SIZE] = {0};
    float bpm_buffer[BPM_AVG_SIZE] = {0};
    uint8_t ir_idx = 0, ir_count = 0;
    uint8_t bpm_idx = 0, bpm_count = 0;

    bool prev_above_avg = false;
    absolute_time_t last_peak_time = get_absolute_time();
    absolute_time_t last_sensor_read = 0;

    while (true) {
        // Sensor alle 20ms abfragen
        if (absolute_time_diff_us(last_sensor_read, get_absolute_time()) / 1000 > 20) {
            last_sensor_read = get_absolute_time();

            uint8_t samples = fifo_available_samples();
            if (samples > 0) {
                uint8_t reg = 0x07;
                uint8_t buffer[6];
                i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
                int read_bytes = i2c_read_blocking(I2C_PORT, MAX30102_ADDR, buffer, 6, false);

                if (read_bytes == 6) {
                    uint32_t ir = ((uint32_t)(buffer[3] & 0x03) << 16) | (buffer[4] << 8) | buffer[5];

                    // Moving Average
                    ir_buffer[ir_idx] = ir;
                    ir_idx = (ir_idx + 1) % MOVING_AVG_SIZE;
                    if (ir_count < MOVING_AVG_SIZE) ir_count++;
                    uint64_t ir_sum = 0;
                    for (uint8_t i = 0; i < ir_count; i++) ir_sum += ir_buffer[i];
                    uint32_t ir_avg = ir_sum / ir_count;

                    if (ir_avg < FINGER_ON_THRESHOLD) {
                       // printf("Kein Finger erkannt.\n");
                        // Servos in Ruheposition
                        for (int i = 0; i < SERVO_COUNT; i++) set_servo_angle(SERVO_PINS[i], 90);
                    } else {
                        // Peak Detection
                        bool curr_above_avg = ir > ir_avg;
                        if (curr_above_avg && !prev_above_avg) {
                            absolute_time_t now = get_absolute_time();
                            int32_t diff_ms = absolute_time_diff_us(last_peak_time, now) / 1000;
                            if (diff_ms > MIN_PEAK_INTERVAL_MS) {
                                float bpm = 60000.0f / diff_ms;
                                if (bpm >= MIN_VALID_BPM && bpm <= MAX_VALID_BPM) {
                                    bpm_buffer[bpm_idx] = bpm;
                                    bpm_idx = (bpm_idx + 1) % BPM_AVG_SIZE;
                                    if (bpm_count < BPM_AVG_SIZE) bpm_count++;

                                    float bpm_sum = 0;
                                    for (uint8_t i = 0; i < bpm_count; i++) bpm_sum += bpm_buffer[i];
                                    float bpm_avg = bpm_sum / bpm_count;

                                    printf("Herzfrequenz: %.1f BPM\n", bpm_avg);

                                    // Servos updaten abhängig von BPM
                                    update_servos(bpm_avg);
                                }
                                last_peak_time = now;
                            }
                        }
                        prev_above_avg = curr_above_avg;
                    }
                }
            }
        }
    }
}
