#include "include/moveServosAlongBPM.h"

// move servos based on bpm 
void move_servos_based_on_bpm(int bpm) {
    if(bpm < 90) {
        servo_set_angle(SERVO_1, 45);
        servo_set_angle(SERVO_2, 45);
        servo_set_angle(SERVO_3, 45);
        servo_set_angle(SERVO_4, 45);
        sleep_ms(1000);
        servo_set_angle(SERVO_1, 0);
        servo_set_angle(SERVO_2, 0);
        servo_set_angle(SERVO_3, 0);
        servo_set_angle(SERVO_4, 0);
    } else if(bpm < 110) {
        servo_set_angle(SERVO_1, 90);
        servo_set_angle(SERVO_2, 90);
        servo_set_angle(SERVO_3, 90);
        servo_set_angle(SERVO_4, 90);
        sleep_ms(1000);
        servo_set_angle(SERVO_1, 0);
        servo_set_angle(SERVO_2, 0);
        servo_set_angle(SERVO_3, 0);
        servo_set_angle(SERVO_4, 0);
    } else if(bpm < 130) {
        servo_set_angle(SERVO_1, 135);
        servo_set_angle(SERVO_2, 135);
        servo_set_angle(SERVO_3, 135);
        servo_set_angle(SERVO_4, 135);
        sleep_ms(1000);
        servo_set_angle(SERVO_1, 0);
        servo_set_angle(SERVO_2, 0);
        servo_set_angle(SERVO_3, 0);
        servo_set_angle(SERVO_4, 0);
    } else {
        servo_set_angle(SERVO_1, 180);
        servo_set_angle(SERVO_2, 180);
        servo_set_angle(SERVO_3, 180);
        servo_set_angle(SERVO_4, 180);
        sleep_ms(1000);
        servo_set_angle(SERVO_1, 0);
        servo_set_angle(SERVO_2, 0);
        servo_set_angle(SERVO_3, 0);
        servo_set_angle(SERVO_4, 0);
    }
}
