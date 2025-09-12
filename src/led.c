#include "Include/led.h"

void init_led(){
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
}

void led_on(){
    gpio_put(LED_PIN, 1);
}

void led_off(){
    gpio_put(LED_PIN, 0);
}