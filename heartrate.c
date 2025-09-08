#include "include/heartrate.h"

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
int bpm_smoothed = 0;
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

int printBPM(){
    uint32_t red, ir;
        if (max30102_read_sample(&red, &ir)) {
            uint32_t now = to_ms_since_boot(get_absolute_time());

            if (detect_heartbeat(ir, now)) {
                bpm_smoothed = smooth_bpm(bpm);
                printf("BPM (raw): %d | BPM (avg): %d\n", bpm, bpm_smoothed);
            }
        }
    sleep_ms(10);
    return bpm_smoothed;
}
