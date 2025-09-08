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
            sleep_ms(2000);
        }
    }
    

    /*
    init_client_wifi();

    max30102_init();
    while (true)
    {
        poll_client_wifi(&client);
        sleep_ms(10);
        int send_bpm = printBPM();
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d\n", bpm); // newline als Trenner
        tcp_client_send_bpm(&client, send_bpm);
    }
     */
        
    /*init_client_wifi();
    
    while (true) {
        poll_client_wifi(&client);
        sleep_ms(10);

        // Test: alle 2 Sekunden eine Nachricht senden
        if (client.connected) {
            int bpm = printBPM();
            tcp_client_send(&client, &bpm);
            sleep_ms(2000);
        }
    }
    */

    return 0;
    
  }
