#include <stdio.h>
#include "servoControl.c"
#include "pico/stdlib.h"

int main() {
    
  servo_init();

  while(1) {
    servo_set_angle(14, 90);
    sleep_ms(1000);
    servo_set_angle(14, 0);
    sleep_ms(1000);

  }
}
