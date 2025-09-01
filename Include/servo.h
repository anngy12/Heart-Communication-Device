#ifndef SERVO_H
#define SERVO_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdbool.h>

#define SERVO_COUNT 4
extern const uint SERVO_PINS[SERVO_COUNT];

// Typ: glatte Sinus-Oszillation um 90°
// angle(t) = 90 + amplitude * sin(2π * phase)
// phase läuft von 0..1 und wird aus Zeitdifferenz fortgeschrieben
typedef struct {
    uint pin;
    float amplitude;           // z.B. 90 -> 0..180
    float bpm;                 // Zieltempo (Vollzyklus 0→180→0)
    absolute_time_t last_ts;   // letzter Zeitstempel
    bool enabled;
    float phase;
    float speed_mul;           // NEU: Geschwindigkeits-Multiplikator (1.0 = 1:1 zu BPM)
} Servo;
void servo_init_all(void);
void set_servo_angle(uint pin, float angle);

// BPM setzen und Oszillation aktivieren (weiche, kontinuierliche Bewegung)
void servo_set_bpm(Servo *servos, float bpm);

// Muss in JEDEM Loop aufgerufen werden, macht die weiche Bewegung
void servo_tick(Servo *servos);

// Stoppt Oszillation und zentriert (90°)
void servo_center_all(Servo *servos);

// NEU: für alle Servos gleichzeitig setzen
void servo_set_speed_multiplier(Servo *servos, float mul);
#endif
