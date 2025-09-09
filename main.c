#include "mosfet.h"
#include "servo.h"
#include "exe_servo_mosfet.h"
#include "include/serverTCP.h"

int main(){
    stdio_init_all();

    init_wifi();
    printf("Server läuft...\n");

    max30102_init();

    Servo  servos[SERVO_COUNT];
    servo_init_and_default(servos);

    Mosfet mosfets[MOSFET_COUNT];
    mosfet_init_all(mosfets);

    bool system_armed = false;   // läuft erst nach 1. BPM

    while (true) {
        poll_wifi();

        int new_bpm;
        if (server_take_bpm(&new_bpm)) {     // neue BPM vom TCP?
            printf("Starte mit externer BPM: %d\n", new_bpm);
            servo_set_bpm(servos, new_bpm);  // Servos auf empfangene BPM setzen
            system_armed = true;             // ab jetzt Servo/MOSFET aktiv
        }

        if (system_armed) {
            servo_mosfet(servos, mosfets);   // läuft nur nach erster BPM
        } else {
            // alles passiv halten
            mosfet_update_all(mosfets);
            // optional: servo_center_all(servos);
        }

        sleep_ms(5);
    }
}
