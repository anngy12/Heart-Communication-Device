#include <stdio.h>
#include "mosfetPeltierControl.c"
#include "servoControl.c"
#include "heartrate.c"


int main() {
    
    stdio_init_all();

    max30102_init();
    printf("MAX30102 initialisiert!\n");

  while(1) {
      printBPM();
  }
}
