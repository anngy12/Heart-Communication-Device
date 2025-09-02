#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"


// Definition of MOSFET pins
#define MOSFET1_PIN 15 
#define MOSFET2_PIN 16
#define MOSFET3_PIN 17
#define MOSFET4_PIN 18


void heat_peltier(int duration){

    // init the GPIO pins
    gpio_init(MOSFET1_PIN);
    gpio_init(MOSFET2_PIN);
    gpio_init(MOSFET3_PIN);
    gpio_init(MOSFET4_PIN);

    // set the GPIO direction (input/output)
    gpio_set_dir(MOSFET1_PIN, true);
    gpio_set_dir(MOSFET2_PIN, true);
    gpio_set_dir(MOSFET3_PIN, true);
    gpio_set_dir(MOSFET4_PIN, true);

    // heat the first two MOSFETS
    gpio_put(MOSFET1_PIN, 1);
    gpio_put(MOSFET2_PIN, 1);

    sleep_ms(duration);

    // turn those two off and turn the other two on
    gpio_put(MOSFET1_PIN, 0);
    gpio_put(MOSFET2_PIN, 0);
    gpio_put(MOSFET3_PIN, 1);
    gpio_put(MOSFET4_PIN, 1);

    sleep_ms(duration);

    // turn those two off
    gpio_put(MOSFET3_PIN, 0);
    gpio_put(MOSFET4_PIN, 0);
}

// heat the peltier if there is a heartbeat detected
void heat_if_heartbeat_detected(int bpm) {
    if (bpm > 0) {
        heat_peltier(1000);  
    }
}