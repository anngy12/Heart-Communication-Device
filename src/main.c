#include "pico/stdlib.h"
#include "max30102.h"
#include "servo.h"
#include <stdio.h>

int main() {
    stdio_init_all();

    // ==== I2C Init ====
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    busy_wait_ms(500);

    // ==== Servo Init ====
    servo_init_all();
    Servo servos[SERVO_COUNT];
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].pin = SERVO_PINS[i];
        servos[i].angle = 90;
        servos[i].last_toggle = get_absolute_time();
        set_servo_angle(servos[i].pin, 90);
    }

    // ==== MAX30102 Init ====
    uint8_t part_id = read_register(0xFF);
    printf("MAX30102 PART ID: 0x%02X\n", part_id);
    if (part_id != 0x15) {
        printf("Sensor nicht erkannt!\n");
        while (true) tight_loop_contents();
    }
    max30102_init();

    // ==== Loop ====
    while (true) {
        uint32_t ir = max30102_read_ir();
        static uint32_t ir_avg = 0;
        ir_avg = (ir_avg * 7 + ir) / 8; // einfacher gleitender Mittelwert

        if (max30102_finger_present(ir_avg)) {
            //printf("[Sensor DBG] Finger erkannt, IR=%lu\n", ir_avg);
            float bpm = 80.0f; // hier später BPM-Berechnung einsetzen
            servo_update_oscillate(servos, bpm);
        } else {
            //printf("[Sensor DBG] Kein Finger erkannt, IR=%lu\n", ir_avg);
            for (int i = 0; i < SERVO_COUNT; i++) {
                set_servo_angle(servos[i].pin, 90);
            }
        }

        sleep_ms(20);
    }
}
