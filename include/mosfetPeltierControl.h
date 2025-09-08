#ifndef MOSFETPELTIERCONTROL_H
#define MOSFETPELTIERCONTROL_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"


// Definition of MOSFET pins
#define MOSFET1_PIN 15 
#define MOSFET2_PIN 16
#define MOSFET3_PIN 17
#define MOSFET4_PIN 18

void heat_peltier(int duration);

void heat_if_heartbeat_detected(int bpm);

#endif // MOSFETPELTIERCONTROL_H