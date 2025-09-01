#include "servo.h"
#include <math.h>

const uint SERVO_PINS[SERVO_COUNT] = {15, 16, 17, 18};

// 50 Hz Servo: 20 ms Periode -> wir nehmen 1 MHz PWM-Taktbasis (125MHz/125)
static const uint32_t SERVO_WRAP = 20000;

static inline float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void servo_init_all(void) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        gpio_set_function(SERVO_PINS[i], GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(SERVO_PINS[i]);
        pwm_set_clkdiv(slice, 125.0f); // 125 MHz / 125 = 1 MHz
        pwm_set_wrap(slice, SERVO_WRAP);
        pwm_set_enabled(slice, true);

        // Start in Mitte
        set_servo_angle(SERVO_PINS[i], 90.0f);
    }
}

void set_servo_angle(uint pin, float angle) {
    angle = clampf(angle, 0.0f, 180.0f);

    // 0.5ms..2.5ms auf 20ms => 2.5%..12.5%
    const float min_ms = 0.5f;
    const float max_ms = 2.5f;
    float pulse_ms = min_ms + (max_ms - min_ms) * (angle / 180.0f);
    uint32_t level = (uint32_t)((pulse_ms / 20.0f) * SERVO_WRAP);
    pwm_set_gpio_level(pin, level);
}

void servo_set_bpm(Servo *servos, float bpm) {
    // sinnvolle Grenzen
    bpm = clampf(bpm, 40.0f, 200.0f);

    // Setze neues Tempo und aktiviere alle Servos
    absolute_time_t now = get_absolute_time();
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].bpm = bpm;
        if (!servos[i].enabled) {
            servos[i].enabled = true;
            servos[i].last_ts = now;
            //servos[i].phase = 0.0f;    
            if (servos[i].amplitude <= 0.0f) servos[i].amplitude = 90.0f;      // volle Auslenkung bei 90.0f 0..180
            if (servos[i].speed_mul <= 0.0f) servos[i].speed_mul = 0.5f; // NEU
        }
    }
}

void servo_tick(Servo *servos) {
    absolute_time_t now = get_absolute_time();

    for (int i = 0; i < SERVO_COUNT; i++) {
        if (!servos[i].enabled) continue;

        // Zeitdelta seit letztem Update in Sekunden (double-Präzision)
        int64_t dt_us = absolute_time_diff_us(servos[i].last_ts, now);
        if (dt_us < 0) dt_us = 0; // falls Clock zurückspringt
        servos[i].last_ts = now;

        double dt_s = dt_us / 1e6;

        // Periodendauer eines Vollzyklus (0→180→0) in Sekunden
        // f = (bpm * speed_mul) / 60  =>  T = 60 / (bpm * speed_mul)
        float mul = (servos[i].speed_mul > 0.0f) ? servos[i].speed_mul : 1.0f;
        double period_s = 60.0 / ((double)servos[i].bpm * (double)mul);
        if (period_s < 0.05) period_s = 0.05; // Schutz


         // Phase fortschreiben: Δphase = dt / T
        double dphi = dt_s / period_s;
        double phase = servos[i].phase + dphi;

        // mod 1.0 (robust)
        phase -= floor(phase);

        servos[i].phase = (float)phase;

        // Sanfte Sinusbewegung um 90°
        // angle = 90 + A * sin(2π * phase)
        double angle = 90.0 + servos[i].amplitude * sin(2.0 * 3.1415 * phase);

        set_servo_angle(servos[i].pin, (float)angle);
    }
}

void servo_center_all(Servo *servos) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].enabled = false;
        servos[i].bpm = 90.0f;
        servos[i].amplitude = 90.0f;
        servos[i].phase = 0.0f;
        servos[i].last_ts = get_absolute_time();
        set_servo_angle(servos[i].pin, 90.0f);
    }
}

void servo_set_speed_multiplier(Servo *servos, float mul) {
    // sinnvolle Grenzen
    if (mul < 0.1f) mul = 0.1f;
    if (mul > 5.0f) mul = 5.0f;
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].speed_mul = mul;
    }
}

