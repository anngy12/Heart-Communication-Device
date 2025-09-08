#ifndef HEARTRATE_H
#define HEARTRATE_H


#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "max_init.h"

#define SAMPLE_RATE 100  // Hz
#define MIN_BPM 40
#define MAX_BPM 180
#define MIN_INTERVAL (60000 / MAX_BPM) 
#define MAX_INTERVAL (60000 / MIN_BPM)
#define AVG_WINDOW 5 // Anzahl der Messungen pro Durchschnittsberechnung

extern uint32_t last_peak_time;
extern int bpm;
extern uint32_t threshold;

extern int bpm_buffer[AVG_WINDOW];
extern int bpm_index;
extern int bpm_count;
extern int bpm_smoothed;

int detect_heartbeat(uint32_t ir_value, uint32_t current_time_ms);

int smooth_bpm(int new_bpm);

int printBPM();

#endif // HEARTRATE_H