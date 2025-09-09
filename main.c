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
    if (server_take_bpm(&new_bpm)) {
        if (new_bpm > 0) {                 // 0 ignorieren
            servo_set_bpm(servos, new_bpm);           // externe BPM übernimmt
            servo_set_actuation_enabled(true);        // ab jetzt bewegen erlaubt
        }
    }

    // WICHTIG: immer laufen lassen -> misst & sendet auch ohne Empfang
    servo_mosfet(servos, mosfets);

    sleep_ms(5);
}

}
