#include "mosfet.h"
#include "servo.h"
#include "include/led.h"
#include "exe_servo_mosfet.h"
#include "include/clientTCP.h"


// Forward-Decl. der neuen Helferfunktion aus serverTCP.c (für minimalen Eingriff ohne Header-Änderung)
int client_ms_since_rx(void);

// Wie lange ohne Daten, bis wir auf 0 stellen (in Millisekunden)
#define NO_DATA_TIMEOUT_MS 4000

bool sensor_finger_present(void);


int main(){
    stdio_init_all();

    init_led();

    init_client_wifi();
    printf("Client läuft...\n");

    max30102_init();

    Servo  servos[SERVO_COUNT];
    servo_init_and_default(servos);

    Mosfet mosfets[MOSFET_COUNT];
    mosfet_init_all(mosfets);

    while (true) {
    poll_client_wifi(&client);

    int new_bpm;
    if (client_take_bpm(&new_bpm)) {
        if (new_bpm > 0) {                 // 0 ignorieren
            servo_set_bpm(servos, new_bpm);           // externe BPM übernimmt
        }
    }

        // Bewegen nur, wenn Daten *und* Finger auf Sensor
    bool have_recent_data = (client_ms_since_rx() <= NO_DATA_TIMEOUT_MS);
    bool finger_on        = sensor_finger_present();
    set_have_data(have_recent_data);

    if (!(have_recent_data && finger_on)) {
            servo_center_all(servos);            // alle auf 0° und disabled
            servo_set_actuation_enabled(false);// kein servo_tick() mehr
            led_off();  
        } else {
            // bei aktiven Daten sicherstellen, dass Bewegung erlaubt ist
            servo_set_actuation_enabled(true);
        }
        if(have_recent_data);
        led_on();

    // WICHTIG: immer laufen lassen -> misst & sendet auch ohne Empfang
    servo_mosfet(servos, mosfets);
    new_bpm = 0;
    sleep_ms(5);
}

}
