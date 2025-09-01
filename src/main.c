#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>

#include "max30102.h"
#include "servo.h"
#include "mosfet.h"

#include <math.h>

// ---- BPM-Detektions-Parameter ----
#define MOVING_AVG_SIZE      8
#define BPM_AVG_SIZE         4
#define MIN_VALID_BPM        60
#define MAX_VALID_BPM        180
#define MIN_PEAK_INTERVAL_MS 500
#define FINGER_ON_THRESHOLD  50000  // IR-Durchschnittsschwelle

int main() {
    stdio_init_all();
    float bpm_target = 90.0f;  // Default, bis erste Messung kommt
    bool finger_on = false;

    // I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    busy_wait_ms(200);

    // Servos
    servo_init_all();
    Servo servos[SERVO_COUNT];
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].pin = SERVO_PINS[i];
        servos[i].enabled = false;
        servos[i].amplitude = 90.0f;   // volle Auslenkung
        servos[i].bpm = 90.0f;         // Default
        servos[i].phase = 0.0f;
        servos[i].last_ts = get_absolute_time();
        set_servo_angle(servos[i].pin, 90.0f);
    }

    // nach der Schleife, die servos[i] füllt:
    for (int i = 0; i < SERVO_COUNT; i++) {
    servos[i].speed_mul = 1.0f;   // Default (1 Zyklus pro Schlag)
}

    servo_set_speed_multiplier(servos, 0.25f);
    servo_set_uniform_phase_step(servos, 0.125f); // 45° Versatz pro Kanal             

    // MOSFETs
    Mosfet mosfets[MOSFET_COUNT];
    mosfet_init_all(mosfets);

    // MAX30102
    uint8_t part_id = max30102_read_reg(0xFF);
    printf("MAX30102 PART ID: 0x%02X\n", part_id);
    if (part_id != 0x15) {
        printf("Sensor nicht erkannt!\n");
        while (1) tight_loop_contents();
    }
    max30102_init();

    // Puffer
    uint32_t ir_buffer[MOVING_AVG_SIZE] = {0};
    uint8_t  ir_idx = 0, ir_count = 0;

    float bpm_buffer[BPM_AVG_SIZE] = {0};
    uint8_t bpm_idx = 0, bpm_count = 0;

    bool prev_above_avg = false;
    absolute_time_t last_peak_time = get_absolute_time();
    absolute_time_t last_sensor_read = 0;

    while (true) {
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

                    finger_on = (ir_avg >= FINGER_ON_THRESHOLD);

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
                                    float bpm_avg = sum_bpm / bpm_count;

                                    printf("Herzfrequenz: %.1f BPM\n", bpm_avg);

                                    // Setzt nur das Zieltempo; das eigentliche Schalten macht servo_tick()
                                    servo_set_bpm(servos, bpm_avg);
                                    bpm_target = bpm_avg; // optional behalten, falls ihr es anderswo braucht
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

        // *** WICHTIG: Servos immer ticken lassen, völlig unabhängig vom Sensor/MOSFET ***
        servo_tick(servos);

        sleep_ms(5);
    }
}
