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

void servo_init_all(void);
void set_servo_angle(uint pin, float angle);

/** Oszilliert alle Servos zeitversetzt (Delay ~300ms), Geschwindigkeit abhängig von bpm.
 *  Wenn bpm außerhalb [60..180], wird 60 angenommen.
 */
void servo_update_oscillate(Servo *servos, float bpm);

#endif
