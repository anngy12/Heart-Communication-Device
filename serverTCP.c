#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

#define WIFI_SSID "PicoAP"
#define WIFI_PASS "12345678"

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb,
                             struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Empfangene Daten ausgeben
    printf("Server empfing: %.*s\n", p->len, (char *)p->payload);

    // Antwort zurücksenden
    char *reply = "Hallo vom Server!";
    tcp_write(tpcb, reply, strlen(reply), TCP_WRITE_FLAG_COPY);

    tcp_recved(tpcb, p->len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    printf("Client verbunden!\n");
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

int main() {
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

    while (true) {
        cyw43_arch_poll(); // WLAN stack
        sleep_ms(10);
    }
}
