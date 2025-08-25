#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include <stdio.h>

absolute_time_t last_action_time = 0;
bool action_in_progress = false;

// ===== MAX30102 wie bisher =====
#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define MAX30102_ADDR 0x57

// ===== Servo (SG92R) =====
#define SERVO_PIN 15   // GPIO für Servo-Signal (bitte anpassen!)
#define MOSFET_PIN 14


// Herzfrequenz-Berechnung
#define MOVING_AVG_SIZE 8
#define BPM_AVG_SIZE 4
#define MIN_VALID_BPM 60
#define MAX_VALID_BPM 180
#define MIN_PEAK_INTERVAL_MS 500
#define FINGER_ON_THRESHOLD 50000

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
    sleep_ms(100);

    uint8_t int_enable1[2] = { 0x02, 0x00 };
    uint8_t int_enable2[2] = { 0x03, 0x00 };
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, int_enable1, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, int_enable2, 2, false);

    uint8_t fifo_cfg[2] = { 0x08, 0x4F };
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, fifo_cfg, 2, false);

    uint8_t mode_cfg[2] = { 0x09, 0x03 };
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, mode_cfg, 2, false);

    uint8_t spO2_cfg[2] = { 0x0A, 0x27 };
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, spO2_cfg, 2, false);

    uint8_t led_red[2] = { 0x0C, 0x10 };
    uint8_t led_ir[2]  = { 0x0D, 0x10 };
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, led_red, 2, false);
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, led_ir, 2, false);
}

// ========= Servo-Funktion =========
void set_servo_angle(uint angle) {
    // SG92R: 0° = ~0.5ms, 180° = ~2.5ms bei 20ms Period
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    float min = 0.5f, max = 2.5f; // ms
    float pulse_ms = min + (max - min) * ((float)angle / 180.0f);
    uint32_t wrap = 20000; // 20ms Period bei 1 MHz clk
    pwm_set_wrap(slice, wrap);
    uint32_t level = (pulse_ms / 20.0f) * wrap;
    pwm_set_gpio_level(SERVO_PIN, level);
}

int main() {
    stdio_init_all();


    gpio_init(MOSFET_PIN);
    gpio_set_dir(MOSFET_PIN, GPIO_OUT);

    // ==== I2C init ====
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    sleep_ms(500);

    // ==== Servo Init ====
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice, 125.0f);  // 125 MHz / 125 = 1 MHz Takt
    pwm_set_wrap(slice, 20000);     // 20 ms Period
    pwm_set_enabled(slice, true);

    // ==== MAX30102 check ====
    uint8_t part_id = read_register(0xFF);
    printf("MAX30102 PART ID: 0x%02X\n", part_id);
    if (part_id != 0x15) {
        printf("Sensor nicht erkannt!\n");
        while (true) tight_loop_contents();
    }
    max30102_init();

    // ==== Buffers ====
    uint32_t ir_buffer[MOVING_AVG_SIZE] = {0};
    float bpm_buffer[BPM_AVG_SIZE] = {0};
    uint8_t ir_idx = 0, ir_count = 0;
    uint8_t bpm_idx = 0, bpm_count = 0;

    bool prev_above_avg = false;
    absolute_time_t last_peak_time = get_absolute_time();

    while (true) {
        uint8_t samples = fifo_available_samples();
        if (samples == 0) {
            sleep_ms(10);
            continue;
        }

        uint8_t reg = 0x07;
        uint8_t buffer[6];
        i2c_write_blocking(I2C_PORT, MAX30102_ADDR, &reg, 1, true);
        int read_bytes = i2c_read_blocking(I2C_PORT, MAX30102_ADDR, buffer, 6, false);
        if (read_bytes != 6) continue;

        uint32_t ir = ((uint32_t)(buffer[3] & 0x03) << 16) | (buffer[4] << 8) | buffer[5];

        ir_buffer[ir_idx] = ir;
        ir_idx = (ir_idx + 1) % MOVING_AVG_SIZE;
        if (ir_count < MOVING_AVG_SIZE) ir_count++;

        uint64_t ir_sum = 0;
        for (uint8_t i = 0; i < ir_count; i++) ir_sum += ir_buffer[i];
        uint32_t ir_avg = ir_sum / ir_count;

        // Debug
        printf("IR: %lu\n", ir);

        if (ir_avg < FINGER_ON_THRESHOLD) {
            printf("Kein Finger erkannt.\n");
            set_servo_angle(180);
            sleep_ms(20);
            continue;
        }

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

                    // === Servo abhängig von BPM bewegen ===
                    if (!action_in_progress && bpm_avg > 100) {
                        set_servo_angle(0); 
                        printf("Servo -> über 100\n");
                    } else if (!action_in_progress && bpm_avg < 90) {
                        // MOSFET einschalten (Gate HIGH)
                        gpio_put(MOSFET_PIN, 1);
                        sleep_ms(1000); // 1 Sekunde warten
                        // MOSFET ausschalten (Gate LOW)
                        gpio_put(MOSFET_PIN, 0);
                        set_servo_angle(0); 
                        sleep_ms(200);
                        set_servo_angle(180);
                        printf("Servo -> unter 90\n");
                        last_action_time = get_absolute_time();
                        action_in_progress = true;
                    } else if (action_in_progress) {
                        int64_t dt = absolute_time_diff_us(last_action_time, get_absolute_time()) / 1000;
                        if (dt > 1000) {
                            gpio_put(MOSFET_PIN, 0);
                            set_servo_angle(0);
                            action_in_progress = false;
                        } 
                    }else {
                        set_servo_angle(180); 
                        printf("Servo -> zwischen 90 und 100\n");
                    }
                }
                last_peak_time = now;
            }
        }
        prev_above_avg = curr_above_avg;
        sleep_ms(20);
    }
}   
