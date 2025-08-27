#include "servo.h"

const uint SERVO_PINS[SERVO_COUNT] = {15, 16, 17, 18};
static const uint32_t SERVO_WRAP = 20000; // 50 Hz PWM

void servo_init_all() {
    for (int i = 0; i < SERVO_COUNT; i++) {
        gpio_set_function(SERVO_PINS[i], GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(SERVO_PINS[i]);
        pwm_set_clkdiv(slice, 125.0f);  // 1 µs Auflösung
        pwm_set_wrap(slice, SERVO_WRAP);
        pwm_set_enabled(slice, true);
    }
}

void set_servo_angle(uint pin, float angle) {
    float min = 0.5f, max = 2.5f;
    float pulse_ms = min + (max - min) * (angle / 180.0f);
    uint slice = pwm_gpio_to_slice_num(pin);
    uint32_t level = (pulse_ms / 20.0f) * SERVO_WRAP;
    pwm_set_gpio_level(pin, level);

    // Debug
    printf("[Servo DBG] Pin %u -> Winkel: %.1f° (PWM=%lu)\n", pin, angle, level);
}

void servo_update_oscillate(Servo *servos, float bpm) {
    float freq = (bpm > 0) ? bpm : 60;
    float base_speed = 1000.0f / (freq / 60.0f);

    for (int i = 0; i < SERVO_COUNT; i++) {
        if (absolute_time_diff_us(servos[i].last_toggle, get_absolute_time()) >
            base_speed * 1000 + i * 200000) {
            servos[i].last_toggle = get_absolute_time();
            servos[i].angle = (servos[i].angle == 0) ? 180 : 0;

            printf("[Servo DBG] Servo %d toggelt -> Ziel %d°\n",
                   i, servos[i].angle);

            set_servo_angle(servos[i].pin, servos[i].angle);
        }
    }
}
