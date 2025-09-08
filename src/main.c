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
        exe_servo_mosfet(servos, mosfets);
        sleep_ms(5);
    }
}
