#ifndef MOVE_SERVOS_ALONG_BPM_H
#define MOVE_SERVOS_ALONG_BPM_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "heartrate.h"
#include "servoControl.h"

void move_servos_based_on_bpm(int bpm);

#endif // MOVE_SERVOS_ALONG_BPM_H