#ifndef MOSFET_H
#define MOSFET_H

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h>

#define MOSFET_COUNT 3
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
    uint32_t on_ms;   // NEU: AN-Dauer
    uint32_t off_ms;  // NEU: AUS-Dauer
} Mosfet;

void mosfet_init_all(Mosfet *m);

// Startet Daueroszillation mit frei wählbaren Zeiten (z. B. 3000, 2000)
void mosfet_start_oscillate(Mosfet *m, uint id, uint32_t on_ms, uint32_t off_ms);

// Zeiten zur Laufzeit ändern (wirksam ab nächstem Umschaltpunkt)
static inline void mosfet_set_durations(Mosfet *m, uint id, uint32_t on_ms, uint32_t off_ms) {
    if (id >= MOSFET_COUNT) return;
    m[id].on_ms  = on_ms;
    m[id].off_ms = off_ms;
}

void mosfet_stop(Mosfet *m, uint id);
void mosfet_update_all(Mosfet *m);

#endif
