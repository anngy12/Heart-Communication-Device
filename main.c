#include <stdio.h>
#include "mosfetPeltierControl.c"
#include "servoControl.c"

int main() {
    while (1) {
        heat_peltier(2000);
    }
}
