#include "servo.h"
#include <math.h>

#define M_PI 3.14159265358979323846


const uint SERVO_PINS[SERVO_COUNT] = {16, 17, 18};
static const uint32_t SERVO_WRAP = 20000; // 50 Hz bei 1 MHz PWM-Basis

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
        set_servo_angle(SERVO_PINS[i], 0);
    }
}

void set_servo_angle(uint pin, float angle) {
    angle = clampf(angle, 0.0f, 180.0f);
    const float min_ms = 0.5f, max_ms = 2.5f;
    float pulse_ms = min_ms + (max_ms - min_ms) * (angle / 180.0f);
    uint32_t level = (uint32_t)((pulse_ms / 20.0f) * SERVO_WRAP);
    pwm_set_gpio_level(pin, level);
}

void servo_set_bpm(Servo *servos, float bpm) {
    bpm = clampf(bpm, 40.0f, 200.0f);
    absolute_time_t now = get_absolute_time();
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].bpm = bpm;
        if (!servos[i].enabled) {
            servos[i].enabled   = true;
            servos[i].last_ts   = now;
            if (servos[i].amplitude <= 0.0f) servos[i].amplitude = 90.0f;
            if (servos[i].speed_mul <= 0.0f) servos[i].speed_mul = 1.0f; // Default
            // phase & phase_offset NICHT anfassen -> Phasing bleibt erhalten
        }
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

void servo_tick(Servo *servos) {
    absolute_time_t now = get_absolute_time();
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (!servos[i].enabled) continue;

        int64_t dt_us = absolute_time_diff_us(servos[i].last_ts, now);
        if (dt_us < 0) dt_us = 0;
        servos[i].last_ts = now;

        double dt_s = dt_us / 1e6;

        // *** HIER: speed_mul berücksichtigt ***
        float bpm   = clampf(servos[i].bpm, 40.0f, 200.0f);
        float mul   = (servos[i].speed_mul > 0.0f) ? servos[i].speed_mul : 1.0f;
        double period_s = 60.0 / ((double)bpm * (double)mul); // T = 60/(BPM*mul)
        if (period_s < 0.05) period_s = 0.05;

        // Phase 0..1 fortschreiben
        double phase = servos[i].phase + dt_s / period_s;
        phase -= floor(phase);
        servos[i].phase = (float)phase;

        // fester Offset addieren (0..1)
        double phase_total = phase + (double)servos[i].phase_offset;
        phase_total -= floor(phase_total);

        // glatte Sinusbewegung
        double angle = 90.0 + servos[i].amplitude * sin(2.0 * M_PI * phase_total);
        set_servo_angle(servos[i].pin, (float)angle);
    }
}

void servo_center_all(Servo *servos) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].enabled = false;  // stoppt Bewegung
        set_servo_angle(servos[i].pin, 0);
        // phase / phase_offset unverändert lassen
    }
}

void servo_set_uniform_phase_step(Servo *servos, float step_0to1) {
    if (step_0to1 <= 0.0f) step_0to1 = 0.25f;
    for (int i = 0; i < SERVO_COUNT; i++) {
        double ph = fmod((double)step_0to1 * i, 1.0);
        servos[i].phase_offset = (float)ph;
    }
}

void servo_set_phase_offsets(Servo *servos, const float *offsets_0to1) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        float ph = offsets_0to1[i];
        while (ph < 0.0f)  ph += 1.0f;
        while (ph >= 1.0f) ph -= 1.0f;
        servos[i].phase_offset = ph;
    }
}

void servo_init_and_default(Servo *servos){
    // Servos
    servo_init_all();
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].pin = SERVO_PINS[i];
        servos[i].enabled = false;
        servos[i].amplitude = 90.0f;   // volle Auslenkung
        servos[i].bpm = 90.0f;         // Default
        servos[i].phase = 0.0f;
        servos[i].last_ts = get_absolute_time();
        set_servo_angle(servos[i].pin, 0);
    }
     // nach der Schleife, die servos[i] füllt:
    for (int i = 0; i < SERVO_COUNT; i++) {
    servos[i].speed_mul = 1.0f;   // Default (1 Zyklus pro Schlag)
    }
    servo_set_speed_multiplier(servos, 0.25f);
    servo_set_uniform_phase_step(servos, 0.125f); // 45° Versatz pro Kanal     
}