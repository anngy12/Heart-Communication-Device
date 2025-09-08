#include "include/clientTCP.h"
#include "include/serverTCP.h"
#include "include/heartrate.h"
#include "include/max_init.h"


int main() {
    sleep_ms(2000); // Warten, bis die serielle Verbindung hergestellt ist

    
    stdio_init_all();

    init_wifi();

    printf("Server läuft...\n");

    max30102_init();

    while (true) { 
        poll_wifi();
        sleep_ms(10);
        if (server.connected) {
            int bpm = printBPM();
            tcp_server_send_bpm(bpm);
            sleep_ms(2000);
        }
    }

    /*
    stdio_init_all();

    init_client_wifi();

    max30102_init();
    while (true)
    {
        poll_client_wifi(&client);
        sleep_ms(10);
        int send_bpm = printBPM();
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d\n", send_bpm);
        tcp_client_send_bpm(&client, send_bpm);
        sleep_ms(2000);
    }
        */

    return 0;
    
  }
