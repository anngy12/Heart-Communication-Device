#ifndef MOSFET_H
#define MOSFET_H

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h>

#define MOSFET_COUNT 4
extern const uint MOSFET_PINS[MOSFET_COUNT];

typedef enum {
    MOSFET_IDLE,
    MOSFET_ON,
    MOSFET_OFF
} MosfetState;

typedef struct {
    uint pin;
    MosfetState state;
    absolute_time_t change_at;
} Mosfet;

void mosfet_init_all(Mosfet *m);
void mosfet_start_oscillate(Mosfet *m, uint id); // startet Daueroszillation
void mosfet_stop(Mosfet *m, uint id);            // stoppt Oszillation
void mosfet_update_all(Mosfet *m);

#endif
