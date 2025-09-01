#include "mosfet.h"

const uint MOSFET_PINS[MOSFET_COUNT] = {10, 11, 12, 13}; // <- an deine Pins anpassen

static inline bool mosfet_time_reached(absolute_time_t t) {
    return absolute_time_diff_us(get_absolute_time(), t) <= 0;
}

void mosfet_init_all(Mosfet *m) {
    for (int i = 0; i < MOSFET_COUNT; i++) {
        m[i].pin = MOSFET_PINS[i];
        m[i].state = MOSFET_IDLE;
        m[i].change_at = 0;
        m[i].on_ms  = 1000; // Default 1s an
        m[i].off_ms = 2000; // Default 2s aus
        gpio_init(m[i].pin);
        gpio_set_dir(m[i].pin, GPIO_OUT);
        gpio_put(m[i].pin, 0);
    }
}

void mosfet_start_oscillate(Mosfet *m, uint id, uint32_t on_ms, uint32_t off_ms) {
    if (id >= MOSFET_COUNT) return;
    // gewünschte Zeiten übernehmen
    m[id].on_ms  = (on_ms  == 0) ? 1 : on_ms;
    m[id].off_ms = off_ms; // darf 0 sein, wenn gewünscht
    if (m[id].state == MOSFET_IDLE) {
        gpio_put(m[id].pin, 1);                 // Start: AN
        m[id].state = MOSFET_ON;
        m[id].change_at = delayed_by_ms(get_absolute_time(), m[id].on_ms);
        //printf("[MOSFET] #%u START -> ON (%lums)\n", id, (unsigned long)m[id].on_ms);
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
        switch (m[i].state) {
            case MOSFET_ON:
                if (mosfet_time_reached(m[i].change_at)) {
                    gpio_put(m[i].pin, 0);
                    m[i].state = MOSFET_OFF;
                    m[i].change_at = delayed_by_ms(now, m[i].off_ms);
                    printf("[MOSFET] #%d -> OFF (%lums)\n", i, (unsigned long)m[i].off_ms);
                }
                break;
            case MOSFET_OFF:
                if (mosfet_time_reached(m[i].change_at)) {
                    gpio_put(m[i].pin, 1);
                    m[i].state = MOSFET_ON;
                    m[i].change_at = delayed_by_ms(now, m[i].on_ms);
                    printf("[MOSFET] #%d -> ON (%lums)\n", i, (unsigned long)m[i].on_ms);
                }
                break;
            default:
                break;
        }
    }
}
