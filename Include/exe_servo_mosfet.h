#pragma once
#include "servo.h"
#include "mosfet.h"

// Führt genau einen Schleifendurchlauf der App-Logik aus.
// Erwartet initialisierte Servos, MOSFETs sowie den MAX30102.
void app_loop_step(Servo servos[], Mosfet mosfets[]);
