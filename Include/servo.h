#ifndef SERVO_H
#define SERVO_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdbool.h>
#include <math.h>

#define M_PI 3.14159265358979323846
#define SERVO_COUNT 4

extern const uint SERVO_PINS[SERVO_COUNT];

typedef struct {
    uint pin;
    float amplitude;        // 90 -> 0..180
    float bpm;              // Vollzyklen/Minute (0→180→0)
    float speed_mul;        // Geschw.-Multiplikator (1.0 = 1:1 zu BPM)
    float phase;            // laufende Phase 0..1
    float phase_offset;     // fester Phasenversatz 0..1
    absolute_time_t last_ts;
    bool enabled;
} Servo;

void servo_init_all(void);
void set_servo_angle(uint pin, float angle);

void servo_set_bpm(Servo *servos, float bpm);          // ändert Tempo, nicht Phase/Offsets
void servo_set_speed_multiplier(Servo *servos, float mul); // 0.1..5.0 empfohlen
void servo_tick(Servo *servos);
void servo_center_all(Servo *servos);

void servo_set_uniform_phase_step(Servo *servos, float step_0to1);
void servo_set_phase_offsets(Servo *servos, const float *offsets_0to1);

void servo_init_and_default();

#endif
