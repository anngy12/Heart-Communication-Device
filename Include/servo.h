#ifndef SERVO_H
#define SERVO_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdio.h>

#define SERVO_COUNT 4
extern const uint SERVO_PINS[SERVO_COUNT];

typedef struct {
    uint pin;
    int angle;
    absolute_time_t last_toggle;
} Servo;

void servo_init_all();
void set_servo_angle(uint pin, float angle);
void servo_update_oscillate(Servo *servos, float bpm);

#endif
