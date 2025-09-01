#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "max_init.c"

#define SAMPLE_RATE 100  // Hz
#define MIN_BPM 40
#define MAX_BPM 180
#define MIN_INTERVAL (60000 / MAX_BPM) 
#define MAX_INTERVAL (60000 / MIN_BPM)
#define AVG_WINDOW 5 // Anzahl der Messungen pro Durchschnittsberechnung

int bpm_buffer[AVG_WINDOW] = {0};
int bpm_index = 0;
int bpm_count = 0;

int smooth_bpm(int new_bpm) {
    bpm_buffer[bpm_index] = new_bpm;
    bpm_index = (bpm_index + 1) % AVG_WINDOW;
    if (bpm_count < AVG_WINDOW) bpm_count++;

    int sum = 0;
    for (int i = 0; i < bpm_count; i++) {
        sum += bpm_buffer[i];
    }
    return sum / bpm_count;
}

uint32_t last_peak_time = 0;
int bpm = 0;
uint32_t threshold = 50000;

int detect_heartbeat(uint32_t ir_value, uint32_t current_time_ms) {
    static int state = 0; 
    int beat_detected = 0;

    // dynamische Schwelle
    static uint32_t avg_ir = 0;
    avg_ir = (avg_ir * 9 + ir_value) / 10;
    threshold = avg_ir + 2000;

    if (state == 0 && ir_value > threshold) {
        state = 1;
        uint32_t dt = current_time_ms - last_peak_time;
        if (dt > MIN_INTERVAL && dt < MAX_INTERVAL && last_peak_time > 0) {
            bpm = 60000 / dt;
            beat_detected = 1;
        }
        last_peak_time = current_time_ms;
    }
    if (state == 1 && ir_value < threshold) {
        state = 0;
    }
    return beat_detected;
}

