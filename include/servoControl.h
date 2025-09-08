#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <stdio.h>
#include <stdint.h>
#include "hardware/pwm.h"
#include "hardware/gpio.h"

#define SERVO_1 14
#define SERVO_2 13
#define SERVO_3 12
#define SERVO_4 11

void servo_init();
void servo_set_angle(int servo, float angle);

#endif // SERVO_CONTROL_H