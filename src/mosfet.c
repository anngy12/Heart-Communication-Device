#include "mosfet.h"

const uint MOSFET_PINS[MOSFET_COUNT] = {10, 11, 12, 13}; // anpassen

static inline bool mosfet_time_reached(absolute_time_t t) {
    return absolute_time_diff_us(get_absolute_time(), t) <= 0;
}

void mosfet_init_all(Mosfet *m) {
    for (int i = 0; i < MOSFET_COUNT; i++) {
        m[i].pin = MOSFET_PINS[i];
        m[i].state = MOSFET_IDLE;
        m[i].change_at = 0;
        gpio_init(m[i].pin);
        gpio_set_dir(m[i].pin, GPIO_OUT);
        gpio_put(m[i].pin, 0);
    }
}

void mosfet_start_oscillate(Mosfet *m, uint id) {
    if (id >= MOSFET_COUNT) return;
    if (m[id].state == MOSFET_IDLE) {
        gpio_put(m[id].pin, 1); // Start mit AN
        m[id].state = MOSFET_ON;
        m[id].change_at = delayed_by_ms(get_absolute_time(), 1000); // 1s an
        printf("[MOSFET] #%u START -> ON\n", id);
    }
}

void mosfet_stop(Mosfet *m, uint id) {
    if (id >= MOSFET_COUNT) return;
    gpio_put(m[id].pin, 0);
    m[id].state = MOSFET_IDLE;
    //printf("[MOSFET] #%u STOP\n", id);
}

void mosfet_update_all(Mosfet *m) {
    absolute_time_t now = get_absolute_time();
    for (int i = 0; i < MOSFET_COUNT; i++) {
        if (m[i].state == MOSFET_ON && mosfet_time_reached(m[i].change_at)) {
            // nach 1s -> AUS
            gpio_put(m[i].pin, 0);
            m[i].state = MOSFET_OFF;
            m[i].change_at = delayed_by_ms(now, 2000); // 2s aus
            printf("[MOSFET] #%d -> OFF (2s)\n", i);
        }
        else if (m[i].state == MOSFET_OFF && mosfet_time_reached(m[i].change_at)) {
            // nach 2s -> wieder AN
            gpio_put(m[i].pin, 1);
            m[i].state = MOSFET_ON;
            m[i].change_at = delayed_by_ms(now, 1000); // 1s an
            printf("[MOSFET] #%d -> ON (1s)\n", i);
        }
    }
}
