#include "servo.h"

const uint SERVO_PINS[SERVO_COUNT] = {15, 16, 17, 18};
static const uint32_t SERVO_WRAP = 20000; // 50 Hz (mit clkdiv 125.0 -> 1 MHz)

void servo_init_all(void) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        gpio_set_function(SERVO_PINS[i], GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(SERVO_PINS[i]);
        pwm_set_clkdiv(slice, 125.0f); // 125 MHz / 125 = 1 MHz -> 20 ms = 20000 Ticks
        pwm_set_wrap(slice, SERVO_WRAP);
        pwm_set_enabled(slice, true);
        set_servo_angle(SERVO_PINS[i], 90);
    }
}

void set_servo_angle(uint pin, float angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    const float min_ms = 0.5f, max_ms = 2.5f;
    float pulse_ms = min_ms + (max_ms - min_ms) * (angle / 180.0f);
    uint32_t level = (uint32_t)((pulse_ms / 20.0f) * SERVO_WRAP);
    pwm_set_gpio_level(pin, level);
    // Debug:
    // printf("[Servo DBG] Pin %u -> %.1f° (lvl=%lu)\n", pin, angle, level);
}

void servo_update_oscillate(Servo *servos, float bpm) {
    if (bpm < 60.0f || bpm > 180.0f) bpm = 60.0f;
    // Dauer einer Halbschwingung (Umschaltintervall) in ms:
    float interval_ms = 60000.0f / bpm; // 60 BPM -> 1000ms pro Toggle (0<->180)
    const int delay_ms = 30; // Phasenverschiebung zwischen Servos

    absolute_time_t now = get_absolute_time();
    for (int i = 0; i < SERVO_COUNT; i++) {
        int64_t dt_ms = absolute_time_diff_us(servos[i].last_toggle, now) / 1000;
        if (dt_ms >= (int)(interval_ms + i * delay_ms)) {
            servos[i].last_toggle = now;
            servos[i].angle = (servos[i].angle == 0) ? 180 : 0;
            set_servo_angle(servos[i].pin, servos[i].angle);
            // printf("[Servo DBG] #%d -> %d°\n", i, servos[i].angle);
        }
    }
}
