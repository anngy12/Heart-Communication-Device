#include <stdio.h>
#include "mosfetPeltierControl.c"
#include "servoControl.c"
#include "heartrate.c"
#include "moveServosAlongBPM.c"

int main() {
    
  stdio_init_all();
  max30102_init();
  servo_init();

  while(1) {
  int now = printBPM();
  move_servos_based_on_bpm(now);
  }
}
