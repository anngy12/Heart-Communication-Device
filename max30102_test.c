#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include <stdio.h>

// ===== MAX30102 wie bisher =====
#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define MAX30102_ADDR 0x57

// ===== Servos =====
#define SERVO_COUNT 4
const uint SERVO_PINS[SERVO_COUNT] = {15, 16, 17, 18};
#define MOSFET_PIN 14

// Herzfrequenz-Berechnung
#define MOVING_AVG_SIZE 8
#define BPM_AVG_SIZE 4
#define MIN_VALID_BPM 60
#define MAX_VALID_BPM 180
#define MIN_PEAK_INTERVAL_MS 500
#define FINGER_ON_THRESHOLD 50000
#define MOSFET_ON_TIME_MS 300

absolute_time_t last_action_time = 0;
bool action_in_progress = false;

// ===== Servo State-Machine =====
typedef struct {
    int current;
    int target;
    int step;
    absolute_time_t last_update;
    int interval_ms;
    uint slice;
} ServoMotion;

ServoMotion servos[SERVO_COUNT];

// ===== Action Queue =====
typedef struct {
    int servo_id;
    int target;
    int interval_ms;
    bool mosfet_on;
} Action;

#define ACTION_QUEUE_SIZE 10
Action action_queue[ACTION_QUEUE_SIZE];
int action_head = 0, action_tail = 0;

// Queue-Handling
bool action_queue_empty() {
    return action_head == action_tail;
}

void enqueue_action(Action a) {
    action_queue[action_tail] = a;
    action_tail = (action_tail + 1) % ACTION_QUEUE_SIZE;
}

Action dequeue_action() {
    Action a = action_queue[action_head];
    action_head = (action_head + 1) % ACTION_QUEUE_SIZE;
    return a;
}

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
    uint8_t reset[2] = { 0x09, 0x40 };
    i2c_write_blocking(I2C_PORT, MAX30102_ADDR, reset, 2, false);
    busy_wait_ms(100);

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

// ========= Servo =========
void set_servo_angle(int servo_id, uint angle) {
    ServoMotion *s = &servos[servo_id];
    float min = 0.5f, max = 2.5f;
    float pulse_ms = min + (max - min) * ((float)angle / 180.0f);
    uint32_t wrap = 20000;
    pwm_set_wrap(s->slice, wrap);
    uint32_t level = (pulse_ms / 20.0f) * wrap;
    pwm_set_gpio_level(SERVO_PINS[servo_id], level);
}

void servo_update(int servo_id) {
    ServoMotion *s = &servos[servo_id];
    if (s->current == s->target) return;
    int64_t dt = absolute_time_diff_us(s->last_update, get_absolute_time()) / 1000;
    if (dt >= s->interval_ms) {
        s->last_update = get_absolute_time();
        s->current += s->step;
        set_servo_angle(servo_id, s->current);
        if ((s->step > 0 && s->current >= s->target) ||
            (s->step < 0 && s->current <= s->target)) {
            s->current = s->target;
            set_servo_angle(servo_id, s->current);
        }
    }
}

void servo_update_all() {
    for (int i = 0; i < SERVO_COUNT; i++) {
        servo_update(i);
    }
}

void servo_move_to(int servo_id, int target, int interval_ms) {
    ServoMotion *s = &servos[servo_id];
    s->target = target;
    s->step = (target > s->current) ? 1 : -1;
    s->interval_ms = interval_ms;
    s->last_update = get_absolute_time();
}

// ===== Action Processing =====
void process_actions() {
    if (!action_queue_empty()) {
        Action a = dequeue_action();
        if (a.servo_id >= 0 && a.servo_id < SERVO_COUNT) {
            servo_move_to(a.servo_id, a.target, a.interval_ms);
        }
        if (a.mosfet_on) {
            gpio_put(MOSFET_PIN, 1);
            last_action_time = get_absolute_time();
            action_in_progress = true;
        }
    }
    if (action_in_progress) {
        int64_t dt = absolute_time_diff_us(last_action_time, get_absolute_time()) / 300;
        if (dt > MOSFET_ON_TIME_MS) {
            gpio_put(MOSFET_PIN, 0);
            enqueue_action((Action){.servo_id=0, .target=0, .interval_ms=5, .mosfet_on=false});
            enqueue_action((Action){.servo_id=1, .target=0, .interval_ms=5, .mosfet_on=false});
            enqueue_action((Action){.servo_id=2, .target=0, .interval_ms=5, .mosfet_on=false});
            enqueue_action((Action){.servo_id=3, .target=0, .interval_ms=5, .mosfet_on=false});
            action_in_progress = false;
        }
    }
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
    busy_wait_ms(500);

    // ==== Servo Init ====
    for (int i = 0; i < SERVO_COUNT; i++) {
        gpio_set_function(SERVO_PINS[i], GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(SERVO_PINS[i]);
        servos[i].slice = slice;
        pwm_set_clkdiv(slice, 125.0f);
        pwm_set_wrap(slice, 20000);
        pwm_set_enabled(slice, true);
        servos[i].current = 90;
        servos[i].target = 90;
        set_servo_angle(i, 90);
    }

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
    absolute_time_t last_sensor_read = 0;

    while (true) {
        // ==== Sensor regelmäßig lesen ====
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

                    ir_buffer[ir_idx] = ir;
                    ir_idx = (ir_idx + 1) % MOVING_AVG_SIZE;
                    if (ir_count < MOVING_AVG_SIZE) ir_count++;

                    uint64_t ir_sum = 0;
                    for (uint8_t i = 0; i < ir_count; i++) ir_sum += ir_buffer[i];
                    uint32_t ir_avg = ir_sum / ir_count;

                    if (ir_avg >= FINGER_ON_THRESHOLD) {
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

                                    // === Aktionen ===
                                    if (!action_in_progress && bpm_avg > 100) {
                                        enqueue_action((Action){.servo_id=0, .target=0, .interval_ms=5, .mosfet_on=false});
                                        enqueue_action((Action){.servo_id=1, .target=180, .interval_ms=5, .mosfet_on=false});
                                        enqueue_action((Action){.servo_id=2, .target=90, .interval_ms=5, .mosfet_on=false});
                                        enqueue_action((Action){.servo_id=3, .target=0, .interval_ms=5, .mosfet_on=false});
                                    } else if (!action_in_progress && bpm_avg < 90) {
                                        enqueue_action((Action){.servo_id=0, .target=180, .interval_ms=5, .mosfet_on=true});
                                        enqueue_action((Action){.servo_id=0, .target=0, .interval_ms=5, .mosfet_on=false});
                                        enqueue_action((Action){.servo_id=1, .target=180, .interval_ms=5, .mosfet_on=false});
                                        enqueue_action((Action){.servo_id=1, .target=0, .interval_ms=5, .mosfet_on=false});
                                        enqueue_action((Action){.servo_id=2, .target=180, .interval_ms=5, .mosfet_on=false});
                                        enqueue_action((Action){.servo_id=3, .target=180, .interval_ms=5, .mosfet_on=false});
                                    }
                                }
                                last_peak_time = now;
                            }
                        }
                        prev_above_avg = curr_above_avg;
                    }
                }
            }
        }

        // ==== Aktionen und Servos verarbeiten ====
        process_actions();
        servo_update_all();
    }
}
