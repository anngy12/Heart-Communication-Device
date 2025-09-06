#include <stdio.h>
#include "servoControl.c"
#include "pico/stdlib.h"
#include "clientTCP.c"
// #include "serverTCP.c"

int main() {
    sleep_ms(2000); // Warten, bis die serielle Verbindung hergestellt ist
     
    /*
    init_wifi();

    printf("Server läuft...\n");

    while (true) {
        // TCP/WLAN-Stack verarbeiten
        poll_wifi();
    }
        */
        
    init_client_wifi();

    while (true) {
        poll_client_wifi(&client);
        sleep_ms(10);

        // Test: alle 2 Sekunden eine Nachricht senden
        static absolute_time_t last = {0};
        if (client.connected && absolute_time_diff_us(last, get_absolute_time()) > 2 * 1000000) {
            tcp_client_send(&client, "Hallo vom Client!");
            last = get_absolute_time();
        }
    }

    return 0;
  }
