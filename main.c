#include "mosfet.h"
#include "servo.h"
#include "exe_servo_mosfet.h"
#include "include/serverTCP.h"

// Forward-Decl. der neuen Helferfunktion aus serverTCP.c (für minimalen Eingriff ohne Header-Änderung)
int server_ms_since_rx(void);

// Wie lange ohne Daten, bis wir auf 0 stellen (in Millisekunden)
#define NO_DATA_TIMEOUT_MS 4000

bool sensor_finger_present(void);

#define I2C_SDA     2
#define I2C_SCL     3

#define HUSB238_ADDR 0x08


// --- I2C HUSB238 Setup ---
void setup_husb238_max_power() {
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    uint8_t buf[3] = {0x30, 0x14, 0x05}; // max Power Beispiel
    i2c_write_blocking(i2c1, HUSB238_ADDR, buf, 3, false);
}

int main(){

    setup_husb238_max_power();
    
    stdio_init_all();

    init_wifi();
    printf("Server läuft...\n");

    max30102_init();

    Servo  servos[SERVO_COUNT];
    servo_init_and_default(servos);

    Mosfet mosfets[MOSFET_COUNT];
    mosfet_init_all(mosfets);

    while (true) {
    poll_wifi();

    int new_bpm;
    if (server_take_bpm(&new_bpm)) {
        if (new_bpm > 0) {                 // 0 ignorieren
            servo_set_bpm(servos, new_bpm);           // externe BPM übernimmt
        }
    }


    // Bewegen nur, wenn Daten *und* Finger auf Sensor
        bool have_recent_data = (server_ms_since_rx() <= NO_DATA_TIMEOUT_MS);
        bool finger_on        = sensor_finger_present();

        if (!(have_recent_data && finger_on)) {
            servo_center_all(servos);            // alle auf 0° und disabled
            servo_set_actuation_enabled(false);  // kein servo_tick() mehr
        } else {
            // bei aktiven Daten sicherstellen, dass Bewegung erlaubt ist
            servo_set_actuation_enabled(true);
        }

    // WICHTIG: immer laufen lassen -> misst & sendet auch ohne Empfang
    servo_mosfet(servos, mosfets);
    new_bpm = 0;
    sleep_ms(5);
}

}
