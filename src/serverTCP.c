#include "include/serverTCP.h"   // deklariert server_last_bpm/server_bpm_new o. server_take_bpm()

/* --- Dateiweite Zustände --- */
tcp_server_t server = {0};
char recv_buf[64];
int recv_pos = 0;

volatile int  server_last_bpm = -1;
volatile bool server_bpm_new  = false;

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        printf("Client hat Verbindung geschlossen\n");
        return ERR_OK;
    }
    if (err != ERR_OK) { pbuf_free(p); return err; }

    // 1) gesamte pbuf-Kette verarbeiten
    for (struct pbuf *q = p; q; q = q->next) {
        const char *data = (const char *)q->payload;
        u16_t len = q->len;

        // 2) sicher anhängen (Overflow-Schutz)
        while (len) {
            u16_t cap = (u16_t)(sizeof(recv_buf) - recv_pos);
            if (cap == 0) { recv_pos = 0; cap = sizeof(recv_buf); }  // reset bei vollem Puffer
            u16_t n = len < cap ? len : cap;

            memcpy(&recv_buf[recv_pos], data, n);
            recv_pos += n;
            data     += n;
            len      -= n;

            // Zeilenweise parsen (wie zuvor)
            char *newline;
            while ((newline = memchr(recv_buf, '\n', recv_pos))) {
                *newline = '\0';                         // String beenden
                if (newline > recv_buf && *(newline-1) == '\r') *(newline-1) = '\0'; // CRLF support

                int bpm = atoi(recv_buf);
                server_last_bpm = bpm;
                server_bpm_new  = true;
                printf("Server empfing BPM: %d\n", bpm);

                // Rest nach vorne schieben
                int remaining = recv_pos - (int)(newline + 1 - recv_buf);
                memmove(recv_buf, newline + 1, remaining);
                recv_pos = remaining;
            }
        }
    }
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}


bool server_take_bpm(int *out_bpm) {
    //if (!server_bpm_new) return false;
    *out_bpm = server_last_bpm;
    server_bpm_new = false;
    return true;
}

void tcp_server_send_bpm(int bpm) {
    if (!server.connected) return;

    char buffer[16];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", bpm);
    cyw43_arch_lwip_begin();
    tcp_write(server.pcb, buffer, len, TCP_WRITE_FLAG_COPY);
    tcp_output(server.pcb);
    cyw43_arch_lwip_end();

    printf("Server sendete BPM: %d\n", bpm);
}


static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if(server.connected) {
        tcp_close(newpcb);
        return ERR_OK;
    }
    printf("Client verbunden!\n");
    server.connected = true;
    server.pcb = newpcb;
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

int init_wifi(){
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }

    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
    printf("AP gestartet: SSID=%s, IP=192.168.4.1\n", WIFI_SSID);

    // TCP-Server starten
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    tcp_bind(pcb, IP_ANY_TYPE, 4242);   // Port 4242
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, tcp_server_accept);

    return 0;
}

void poll_wifi(){
    cyw43_arch_poll();
    sleep_ms(10);
}