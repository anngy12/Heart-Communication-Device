#include "exe_servo_mosfet.h"

// Zustände nur einmal anlegen (static = Dateibereich, bleiben über Aufrufe hinweg erhalten)
static uint32_t ir_buffer[MOVING_AVG_SIZE] = {0};
static uint8_t  ir_idx = 0, ir_count = 0;

static float bpm_buffer[BPM_AVG_SIZE] = {0};
static uint8_t bpm_idx = 0, bpm_count = 0;

static bool prev_above_avg = false;
static absolute_time_t last_peak_time;
static absolute_time_t last_sensor_read;

float bpm_avg = 0;

static bool init_done = false;

void servo_mosfet(Servo servos[], Mosfet mosfets[])
{
    if (!init_done) {
        last_peak_time = get_absolute_time();
        last_sensor_read = get_absolute_time();
        init_done = true;
    }

    // === Sensor regelmäßig lesen (alle ~20ms) ===
    if (absolute_time_diff_us(last_sensor_read, get_absolute_time()) / 1000 > 20) {
        last_sensor_read = get_absolute_time();

        if (max30102_fifo_available() > 0) {
            uint32_t ir = max30102_read_ir_sample();
            if (ir > 0) {
                // Moving Average über IR
                ir_buffer[ir_idx] = ir;
                ir_idx = (ir_idx + 1) % MOVING_AVG_SIZE;
                if (ir_count < MOVING_AVG_SIZE) ir_count++;

                uint64_t sum = 0;
                for (uint8_t i = 0; i < ir_count; i++) sum += ir_buffer[i];
                uint32_t ir_avg = (ir_count ? (sum / ir_count) : 0);

                bool finger_on = (ir_avg >= FINGER_ON_THRESHOLD);

                if (!finger_on) {
                    // Servos sauber stoppen & zentrieren:
                    servo_center_all(servos);

                    // MOSFETs stoppen
                    for (int i = 0; i < MOSFET_COUNT; i++) mosfet_stop(mosfets, i);

                    prev_above_avg = false; // Peak-Detektion zurücksetzen
                } else {
                    // MOSFETs: 5000 ms AN / 500 ms AUS
                    for (int i = 0; i < MOSFET_COUNT; i++)
                        mosfet_start_oscillate(mosfets, i, 5000, 500);

                    // Peak-Detektion
                    bool curr_above_avg = ir > ir_avg;
                    if (curr_above_avg && !prev_above_avg) {
                        absolute_time_t now = get_absolute_time();
                        int32_t diff_ms = absolute_time_diff_us(last_peak_time, now) / 1000;
                        if (diff_ms > MIN_PEAK_INTERVAL_MS) {
                            float bpm_instant = 60000.0f / diff_ms;
                            if (bpm_instant >= MIN_VALID_BPM && bpm_instant <= MAX_VALID_BPM) {
                                bpm_buffer[bpm_idx] = bpm_instant;
                                bpm_idx = (bpm_idx + 1) % BPM_AVG_SIZE;
                                if (bpm_count < BPM_AVG_SIZE) bpm_count++;

                                float sum_bpm = 0;
                                for (uint8_t i = 0; i < bpm_count; i++) sum_bpm += bpm_buffer[i];
                                bpm_avg = sum_bpm / bpm_count;

                                printf("Herzfrequenz: %.1f BPM\n", bpm_avg);

                                // Setzt nur das Zieltempo; das eigentliche Schalten macht servo_tick()
                                servo_set_bpm(servos, bpm_avg);
                            }
                            last_peak_time = now;
                        }
                    }
                    prev_above_avg = curr_above_avg;
                }
            }
        }
    }

    // MOSFET-State-Maschine stets updaten
    mosfet_update_all(mosfets);

    // Servos immer ticken lassen
    servo_tick(servos);
}
