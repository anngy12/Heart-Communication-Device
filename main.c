#include "mosfet.h"
#include "servo.h"
#include "exe_servo_mosfet.h"

int main(){
    stdio_init_all();

    void max30102_init(void);

    Servo servos[SERVO_COUNT];
    servo_init_and_default(servos);

    Mosfet mosfets[MOSFET_COUNT];
    mosfet_init_all(mosfets);
    

    while (true) {
        servo_mosfet(servos, mosfets);
        sleep_ms(5);
    }
}