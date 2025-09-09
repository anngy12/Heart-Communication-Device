#pragma once
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "max30102.h"
#include "servo.h"
#include "mosfet.h"

// ---- BPM-Detektions-Parameter ----
#define MOVING_AVG_SIZE      8
#define BPM_AVG_SIZE         4
#define MIN_VALID_BPM        60
#define MAX_VALID_BPM        180
#define MIN_PEAK_INTERVAL_MS 500
#define FINGER_ON_THRESHOLD  50000  // IR-Durchschnittsschwelle


// Führt genau einen Schleifendurchlauf der App-Logik aus.
// Erwartet initialisierte Servos, MOSFETs sowie den MAX30102.
void servo_mosfet(Servo servos[], Mosfet mosfets[]);
