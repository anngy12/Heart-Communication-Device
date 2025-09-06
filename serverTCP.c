#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

#define WIFI_SSID "BPM"
#define WIFI_PASS "123456789"

typedef struct {
    struct tcp_pcb *pcb;
    bool connected;
} tcp_server_t;

tcp_server_t server = {0};

// receive messages from client
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb,
                             struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        server.connected = false;
        printf("Client hat Verbindung geschlossen\n");
        return ERR_OK;
    }

    // Empfangene Daten ausgeben
    printf("Server empfing: %.*s\n", p->len, (char *)p->payload);

    tcp_recved(tpcb, p->len);
    pbuf_free(p);
    return ERR_OK;
}


// send messages to client
static void tcp_server_send(void *arg, struct tcp_pcb *tpcb, u16_t len, int mode) {
    cyw43_arch_lwip_begin();
    tcp_write(tpcb, "c", 18, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    cyw43_arch_lwip_end();
    printf("Server sendet: Modus %d\n", mode);
}

// accept connection from client
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
}

void poll_wifi(){
    cyw43_arch_poll();
    sleep_ms(10);
}