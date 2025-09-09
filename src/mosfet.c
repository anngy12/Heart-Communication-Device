#include "mosfet.h"

const uint MOSFET_PINS[MOSFET_COUNT] = {11, 12, 13}; // <- an deine Pins anpassen

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

void mosfet_start_oscillate_delayed(Mosfet *m, uint id,
                                    uint32_t on_ms, uint32_t off_ms,
                                    uint32_t start_delay_ms)
{
    if (id >= MOSFET_COUNT) return;

    m[id].on_ms  = on_ms  ? on_ms  : 1;
    m[id].off_ms = off_ms; // 0 erlaubt

    if (m[id].state == MOSFET_IDLE) {
        if (start_delay_ms == 0) {
            // sofort starten
            gpio_put(m[id].pin, 1);
            m[id].state     = MOSFET_ON;
            m[id].change_at = delayed_by_ms(get_absolute_time(), m[id].on_ms);
        } else {
            // erst später starten
            m[id].state     = MOSFET_WAIT;
            m[id].change_at = delayed_by_ms(get_absolute_time(), start_delay_ms);
            // gpio bleibt aus bis zum Zeitpunkt
        }
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
    for (uint i = 0; i < MOSFET_COUNT; ++i) {
        switch (m[i].state) {

        case MOSFET_WAIT:
            // Startzeit erreicht?
            if (absolute_time_diff_us(m[i].change_at, now) >= 0) {
                gpio_put(m[i].pin, 1);
                m[i].state     = MOSFET_ON;
                m[i].change_at = delayed_by_ms(now, m[i].on_ms);
            }
            break;

        case MOSFET_ON:
            if (absolute_time_diff_us(m[i].change_at, now) >= 0) {
                gpio_put(m[i].pin, 0);
                m[i].state     = MOSFET_OFF;
                m[i].change_at = delayed_by_ms(now, m[i].off_ms);
            }
            break;

        case MOSFET_OFF:
            if (absolute_time_diff_us(m[i].change_at, now) >= 0) {
                gpio_put(m[i].pin, 1);
                m[i].state     = MOSFET_ON;
                m[i].change_at = delayed_by_ms(now, m[i].on_ms);
            }
            break;

        default: break;
        }
    }
}

