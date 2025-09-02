#include <stdio.h>
#include "mosfetPeltierControl.c"
#include "servoControl.c"
#include "heartrate.c"

int main() {
    
    stdio_init_all();
    printf("Starte Herzfrequenzmessung mit MAX30102...\n");

    max30102_init();
    printf("MAX30102 initialisiert!\n");

    while (1) {
        
    }

}
