#include <stdio.h>
#include "mosfetPeltierControl.c"
#include "servoControl.c"
#include "heartrate.c"

int main() {
    
    stdio_init_all();
    printf("Starte Herzfrequenzmessung mit MAX30102...\n");

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    max30102_init();
    printf("MAX30102 initialisiert!\n");

    while (1) {
        uint32_t red, ir;
        if (max30102_read_sample(&red, &ir)) {
            uint32_t now = to_ms_since_boot(get_absolute_time());

            if (detect_heartbeat(ir, now)) {
                int bpm_smoothed = smooth_bpm(bpm);
                printf("❤️ Herzschlag erkannt! BPM (raw): %d | BPM (avg): %d\n", bpm, bpm_smoothed);
            }
        }
        sleep_ms(10);
    }

}
