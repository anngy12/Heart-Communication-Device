#pragma once
#include "servo.h"
#include "mosfet.h"

// Führt genau einen Schleifendurchlauf der App-Logik aus.
// Erwartet initialisierte Servos, MOSFETs sowie den MAX30102.
void servo_mosfet(Servo servos[], Mosfet mosfets[]);
