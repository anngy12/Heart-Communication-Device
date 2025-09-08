#include "include/servoControl.h"

void servo_init() {
    gpio_set_function(SERVO_1, GPIO_FUNC_PWM);
    gpio_set_function(SERVO_2, GPIO_FUNC_PWM);
    gpio_set_function(SERVO_3, GPIO_FUNC_PWM);
    gpio_set_function(SERVO_4, GPIO_FUNC_PWM);

    uint slice_num_1 = pwm_gpio_to_slice_num(SERVO_1);
    uint slice_num_2 = pwm_gpio_to_slice_num(SERVO_2);
    uint slice_num_3 = pwm_gpio_to_slice_num(SERVO_3);
    uint slice_num_4 = pwm_gpio_to_slice_num(SERVO_4);

    // 20ms Periode = 50Hz, also eine Messperiode
    pwm_set_wrap(slice_num_1, 20000); 
    pwm_set_wrap(slice_num_2, 20000);
    pwm_set_wrap(slice_num_3, 20000);
    pwm_set_wrap(slice_num_4, 20000);

     // ergibt 1 MHz PWM-Basisfrequenz
    pwm_set_clkdiv(slice_num_1, 125.0);
    pwm_set_clkdiv(slice_num_2, 125.0);
    pwm_set_clkdiv(slice_num_3, 125.0);
    pwm_set_clkdiv(slice_num_4, 125.0);

    pwm_set_enabled(slice_num_1, true);
    pwm_set_enabled(slice_num_2, true);
    pwm_set_enabled(slice_num_3, true);
    pwm_set_enabled(slice_num_4, true);
}

void servo_set_angle(int servo, float angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    uint16_t pulse_width = 1000 + (angle / 180.0f) * 1000;
    uint slice_num = pwm_gpio_to_slice_num(servo);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(servo), pulse_width);
}

